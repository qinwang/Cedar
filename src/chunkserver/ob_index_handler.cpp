/*
 * ob_index_handler.cpp
 *
 *  Created on: 2015年12月5日
 *      Author: longfei
 *  longfei1lantern@gmail.com
 */

#include "ob_index_handler.h"
#include "ob_tablet_manager.h"
#include "ob_disk_manager.h"
#include "common/file_directory_utils.h"

namespace oceanbase
{
  namespace chunkserver
  {
    using namespace sstable;

    ObIndexHandler::ObIndexHandler(ObIndexHandlePool *pool,
        common::ObMergerSchemaManager *schema_mgr, ObTabletManager* tablet_mgr) :
        handle_pool_(pool), manager_(schema_mgr), tablet_manager_(tablet_mgr)
    {
    }

    ObIndexHandler::~ObIndexHandler()
    {
      // TODO Auto-generated destructor stub
    }

    int ObIndexHandler::create_new_sstable(uint64_t table_id,
        int32_t disk)
    {
      int ret = OB_SUCCESS;
      sstable_id_.sstable_file_id_ =
          tablet_manager_->allocate_sstable_file_seq();
      sstable_id_.sstable_file_offset_ = 0;
      int32_t disk_no = disk;
      int64_t sstable_block_size = OB_DEFAULT_SSTABLE_BLOCK_SIZE;
      bool is_sstable_exist = false;
      sstable_schema_.reset();
      //sstable_writer_.reset();
      if (disk_no < 0)
      {
        TBSYS_LOG(ERROR, "does't have enough disk space for static index");
        sstable_id_.sstable_file_id_ = 0;
        ret = OB_CS_OUTOF_DISK_SPACE;
      }
      //const ObSchemaManagerV2* mgrv2 = manager_->get_user_schema(0);
      if (NULL == current_schema_manager_)
      {
        TBSYS_LOG(ERROR, "faild to get schema manager!");
        ret = OB_SCHEMA_ERROR;
      }
      else if (NULL
          == (new_table_schema_ = current_schema_manager_->get_table_schema(
              table_id)))
      {
        TBSYS_LOG(ERROR, "failed to get table[%ld] schema", table_id);
        ret = OB_SCHEMA_ERROR;
      }
      else if (OB_SUCCESS
          != (ret = fill_sstable_schema(table_id, *current_schema_manager_,
              sstable_schema_)))
      {
        TBSYS_LOG(ERROR, "fill sstable schema error");
      }
      else
      {
        if (NULL != new_table_schema_ && new_table_schema_->get_block_size() > 0
            && 64 != new_table_schema_->get_block_size())
        {
          sstable_block_size = new_table_schema_->get_block_size();
        }
      }
      if (OB_SUCCESS == ret)
      {
        do
        {
          sstable_id_.sstable_file_id_ = (sstable_id_.sstable_file_id_ << 8)
              | (disk_no & 0xff);

          if ((OB_SUCCESS == ret)
              && (ret = get_sstable_path(sstable_id_, path_, sizeof(path_)))
                  != OB_SUCCESS)
          {
            TBSYS_LOG(ERROR,
                "Create Index : can't get the path of new sstable");
            ret = OB_ERROR;
          }

          if (OB_SUCCESS == ret)
          {
            is_sstable_exist = FileDirectoryUtils::exists(path_);
            if (is_sstable_exist)
            {
              sstable_id_.sstable_file_id_ =
                  tablet_manager_->allocate_sstable_file_seq();
            }
          }
        } while (OB_SUCCESS == ret && is_sstable_exist);

        if (OB_SUCCESS == ret)
        {
          //TBSYS_LOG(INFO,"test::whx create new sstable, sstable_path:%s,"
          // "version=%ld, block_size=%ld,sstable_id=%ld\n",
          //path_,
          // frozen_version_, sstable_block_size,sstable_id_.sstable_file_id_);
          path_string_.assign_ptr(path_,
              static_cast <int32_t>(strlen(path_) + 1));
          compressor_string_.assign_ptr(
              const_cast <char*>(new_table_schema_->get_compress_func_name()),
              static_cast <int32_t>(sizeof(new_table_schema_->get_compress_func_name())
                  + 1));
          int64_t element_count = DEFAULT_ESTIMATE_ROW_COUNT;
          if ((ret = sstable_writer_.create_sstable(sstable_schema_,
              path_string_, compressor_string_, frozen_version_,
              OB_SSTABLE_STORE_DENSE, sstable_block_size, element_count))
              != OB_SUCCESS)
          {
            if (OB_IO_ERROR == ret)
              tablet_manager_->get_disk_manager().set_disk_status(disk_no,
                  DISK_ERROR);
            TBSYS_LOG(ERROR, "Merge : create sstable failed : [%d]", ret);
          }
        }
      }
      return ret;
    }

    int ObIndexHandler::fill_sstable_schema(const uint64_t table_id,
        const common::ObSchemaManagerV2& common_schema,
        sstable::ObSSTableSchema& sstable_schema)
    {
      return build_sstable_schema(table_id, common_schema, sstable_schema,
          false);
    }

    int ObIndexHandler::trans_row_to_sstrow(ObRowDesc &desc, const ObRow &row,
           ObSSTableRow &sst_row)
       {
         int ret = OB_SUCCESS;
         uint64_t tid = OB_INVALID_ID;
         uint64_t cid = OB_INVALID_ID;
         const ObObj *obj = NULL;
         //TBSYS_LOG(INFO, "test::whx row = %s",to_cstring(row));
         if (NULL == new_table_schema_)
         {
           TBSYS_LOG(WARN, "null pointer of table schema");
           ret = OB_INVALID_ARGUMENT;
         }
         else
         {
           row_.clear();
           row_.set_rowkey_obj_count(desc.get_rowkey_cell_count());
         }
         for (int64_t i = 0; OB_SUCCESS == ret && i < desc.get_column_num(); i++)
         {
           if (OB_SUCCESS != (ret = desc.get_tid_cid(i, tid, cid)))
           {
             TBSYS_LOG(WARN, "failed in get tid_cid from row desc,ret[%d]", ret);
             break;
           }
           else if (OB_INDEX_VIRTUAL_COLUMN_ID == cid)
           {
             ObObj vi_obj;
             vi_obj.set_null();
             ret = row_.add_obj(vi_obj);
           }
           else if (OB_SUCCESS != (ret = row.get_cell(tid, cid, obj)))
           {
             TBSYS_LOG(WARN,
                 "get cell from obrow failed!tid[%ld],cid[%ld],ret[%d]", tid, cid,
                 ret);
             break;
           }
           if (OB_SUCCESS == ret && OB_INDEX_VIRTUAL_COLUMN_ID != cid)
           {
             if (NULL == obj)
             {
               TBSYS_LOG(WARN, "obj pointer cannot be NULL!");
               ret = OB_INVALID_ARGUMENT;
               break;
             }
             else if (OB_SUCCESS != (ret = sst_row.add_obj(*obj)))
             {
               TBSYS_LOG(WARN, "set cell for index row failed,ret = %d", ret);
               break;
             }
             else
             {
               //TBSYS_LOG(INFO,"test::whx add obj[%s] success",to_cstring(*obj));
             }
           }
         }
         return ret;
       }

    int ObIndexHandler::save_current_row()
    {
      int ret = OB_SUCCESS;
      /*if (true)
      {
        //TBSYS_LOG(DEBUG, "current row expired.");
        //hex_dump(row_.get_row_key().ptr(), row_.get_row_key().length(), false, TBSYS_LOG_LEVEL_DEBUG);
      }
      else
      */
      if ((ret = sstable_writer_.append_row(row_,current_sstable_size_)) != OB_SUCCESS )
      {
        TBSYS_LOG(ERROR, "write_index : append row failed [%d], this row_, obj count:%ld, "
                         "table:%lu, group:%lu",
            ret, row_.get_obj_count(), row_.get_table_id(), row_.get_column_group_id());
        for(int32_t i=0; i<row_.get_obj_count(); ++i)
        {
          row_.get_obj(i)->dump(TBSYS_LOG_LEVEL_ERROR);
        }
      }
      return ret;
    }

  } /* namespace chunkserver */
} /* namespace oceanbase */
