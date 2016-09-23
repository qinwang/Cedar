/**
 * Copyright (C) 2013-2016 DaSE .
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * @file ob_trans_executor.h
 * @brief TransExecutor
 *     modify by guojinwei: support multiple clusters for HA by
 *     adding or modifying some functions, member variables
 *
 * @version CEDAR 0.2 
 * @author guojinwei <guojinwei@stu.ecnu.edu.cn>
 * @date 2015_12_30
 */
////===================================================================
 //
 // ob_trans_executor.h updateserver / Oceanbase
 //
 // Copyright (C) 2010, 2013 Taobao.com, Inc.
 //
 // Created on 2012-08-30 by Yubai (yubai.lk@taobao.com)
 //
 // -------------------------------------------------------------------
 //
 // Description
 //
 //
 // -------------------------------------------------------------------
 //
 // Change Log
 //
////====================================================================

#ifndef  OCEANBASE_UPDATESERVER_TRANS_EXECUTOR_H_
#define  OCEANBASE_UPDATESERVER_TRANS_EXECUTOR_H_
#include "common/ob_define.h"
#include "common/ob_packet.h"
#include "common/ob_spin_lock.h"
#include "common/data_buffer.h"
#include "common/ob_mod_define.h"
#include "common/ob_queue_thread.h"
#include "common/ob_ack_queue.h"
#include "common/ob_ticket_queue.h"
#include "common/ob_fifo_stream.h"
#include "sql/ob_physical_plan.h"
#include "sql/ob_ups_result.h"
#include "ob_session_mgr.h"
#include "common/ob_fifo_allocator.h"
#include "ob_sessionctx_factory.h"
#include "ob_lock_mgr.h"
#include "ob_util_interface.h"
#include "ob_ups_phy_operator_factory.h"

namespace oceanbase
{
  namespace updateserver
  {
    class TransHandlePool : public common::S2MQueueThread
    {
      public:
        TransHandlePool() : affinity_start_cpu_(-1), affinity_end_cpu_(-1) {};
        virtual ~TransHandlePool() {};
      public:
        void handle(void *ptask, void *pdata)
        {
          static volatile uint64_t cpu = 0;
          rebind_cpu(affinity_start_cpu_, affinity_end_cpu_, cpu);
          handle_trans(ptask, pdata);
        };
        void *on_begin()
        {
          return on_trans_begin();
        };
        void on_end(void *ptr)
        {
          on_trans_end(ptr);
        };
        void set_cpu_affinity(const int64_t start, const int64_t end)
        {
          affinity_start_cpu_ = start;
          affinity_end_cpu_ = end;
        };
      public:
        virtual void handle_trans(void *ptask, void *pdata) = 0;
        virtual void *on_trans_begin() = 0;
        virtual void on_trans_end(void *ptr) = 0;
      private:
        int64_t affinity_start_cpu_;
        int64_t affinity_end_cpu_;
    };

    class CommitEndHandlePool : public common::S2MQueueThread
    {
      public:
        CommitEndHandlePool() {};
        virtual ~CommitEndHandlePool() {};
      public:
        void handle(void *ptask, void *pdata)
        {
          handle_trans(ptask, pdata);
        };
        void *on_begin()
        {
          return on_trans_begin();
        };
        void on_end(void *ptr)
        {
          on_trans_end(ptr);
        };
      public:
        virtual void handle_trans(void *ptask, void *pdata) = 0;
        virtual void *on_trans_begin() = 0;
        virtual void on_trans_end(void *ptr) = 0;
    };

    class TransCommitThread : public SeqQueueThread
    {
      public:
        TransCommitThread() : affinity_cpu_(-1) {};
        virtual ~TransCommitThread() {};
      public:
        void handle(void *ptask, void *pdata)
        {
          rebind_cpu(affinity_cpu_);
          handle_commit(ptask, pdata);
        };
        void *on_begin()
        {
          return on_commit_begin();
        };
        void on_end(void *ptr)
        {
          on_commit_end(ptr);
        };
        void on_idle()
        {
          on_commit_idle();
        };
        void on_push_fail(void* task)
        {
          on_commit_push_fail(task);
        }
        void set_cpu_affinity(const int64_t cpu)
        {
          affinity_cpu_ = cpu;
        };
      public:
        virtual void handle_commit(void *ptask, void *pdata) = 0;
        virtual void *on_commit_begin() = 0;
        virtual void on_commit_end(void *ptr) = 0;
        virtual void on_commit_push_fail(void* ptr) = 0;
        virtual void on_commit_idle() = 0;
        virtual int64_t get_seq(void* task) = 0;
      private:
        int64_t affinity_cpu_;
    };

    class TransExecutor : public TransHandlePool, public TransCommitThread, public CommitEndHandlePool, public IObAsyncClientCallback
    {
      struct TransParamData
      {
        TransParamData() : mod(common::ObModIds::OB_UPS_PHYPLAN_ALLOCATOR),
                           allocator(2 * OB_MAX_PACKET_LENGTH, mod)
                           //allocator(common::ModuleArena::DEFAULT_PAGE_SIZE, mod)
        {
        };
        common::ObMutator mutator;
        common::ObGetParam get_param;
        common::ObScanParam scan_param;
        common::ObScanner scanner;
        common::ObCellNewScanner new_scanner;
        ObUpsPhyOperatorFactory phy_operator_factory;
        sql::ObPhysicalPlan phy_plan;
        common::ModulePageAllocator mod;
        common::ModuleArena allocator;
        char cbuffer[OB_MAX_PACKET_LENGTH];
        common::ObDataBuffer buffer;
      };
      struct CommitParamData
      {
        char cbuffer[OB_MAX_PACKET_LENGTH];
        common::ObDataBuffer buffer;
      };
      struct Task
      {
        common::ObPacket pkt;
        ObTransID sid;
        onev_addr_e src_addr;
        void reset()
        {
          sid.reset();
        };
      };
      static const int64_t TASK_QUEUE_LIMIT = 100000;
      static const int64_t FINISH_THREAD_IDLE = 5000;
      static const int64_t ALLOCATOR_TOTAL_LIMIT = 10L * 1024L * 1024L * 1024L;
      static const int64_t ALLOCATOR_HOLD_LIMIT = ALLOCATOR_TOTAL_LIMIT / 2;
      static const int64_t ALLOCATOR_PAGE_SIZE = 4L * 1024L * 1024L;
      static const int64_t MAX_RO_NUM = 100000;
      static const int64_t MAX_RP_NUM = 10000;
      static const int64_t MAX_RW_NUM = 40000;
      static const int64_t QUERY_TIMEOUT_RESERVE = 20000;
      static const int64_t TRY_FREEZE_INTERVAL = 1000000;
      static const int64_t MAX_BATCH_NUM = 1024;
      static const int64_t FLUSH_QUEUE_SIZE = 1024L * 1024L;
      typedef void (*packet_handler_pt)(common::ObPacket &pkt, common::ObDataBuffer &buffer);
      typedef bool (*trans_handler_pt)(TransExecutor &host, Task &task, TransParamData &pdata);
      typedef bool (*commit_handler_pt)(TransExecutor &host, Task &task, CommitParamData &pdata);
      public:
        TransExecutor(ObUtilInterface &ui);
        ~TransExecutor();
      public:
        int init(const int64_t trans_thread_num,
                const int64_t trans_thread_start_cpu,
                const int64_t trans_thread_end_cpu,
                const int64_t commit_thread_cpu,
                const int64_t commit_end_thread_num);
        void destroy();
      public:
        void handle_packet(common::ObPacket &pkt);

        void handle_trans(void *ptask, void *pdata);
        void *on_trans_begin();
        void on_trans_end(void *ptr);

        void handle_commit(void *ptask, void *pdata);
        void *on_commit_begin();
        void on_commit_push_fail(void* ptr);
        void on_commit_end(void *ptr);
        void on_commit_idle();
        int64_t get_seq(void* ptr);
        int64_t get_commit_queue_len();
        int handle_response(ObAckQueue::WaitNode& node);
        int on_ack(ObAckQueue::WaitNode& node);
        int handle_flushed_log_();

        SessionMgr &get_session_mgr() {return session_mgr_;};
        LockMgr &get_lock_mgr() {return lock_mgr_;};
        void log_trans_info() const;
        int &thread_errno();
        int64_t &batch_start_time();
        //add chujiajia [log synchronization][multi_cluster] 20160606:b
        /**
         * @brief handle uncommited session list after master switch to slave
         * @return OB_SUCCESS if success
         */
        int handle_uncommited_session_list_after_switch();
        //add:e
      private:
        bool handle_in_situ_(const int pcode);
        int push_task_(Task &task);
        bool wait_for_commit_(const int pcode);
        bool is_only_master_can_handle(const int pcode);

        int get_session_type(const ObTransID& sid, SessionType& type, const bool check_session_expired);
        int handle_commit_end_(Task &task, common::ObDataBuffer &buffer);

        bool handle_start_session_(Task &task, common::ObDataBuffer &buffer);
        bool handle_end_session_(Task &task, common::ObDataBuffer &buffer);
        bool handle_write_trans_(Task &task, common::ObMutator &mutator, common::ObNewScanner &scanner);
        bool handle_phyplan_trans_(Task &task,
                                  ObUpsPhyOperatorFactory &phy_operator_factory,
                                  sql::ObPhysicalPlan &phy_plan,
                                  common::ObNewScanner &new_scanner,
                                  common::ModuleArena &allocator,
                                  ObDataBuffer& buffer);
        void handle_get_trans_(common::ObPacket &pkt,
                              common::ObGetParam &get_param,
                              common::ObScanner &scanner,
                              common::ObCellNewScanner &new_scanner,
                              common::ObDataBuffer &buffer);
        void handle_scan_trans_(common::ObPacket &pkt,
                                common::ObScanParam &scan_param,
                                common::ObScanner &scanner,
                                common::ObCellNewScanner &new_scanner,
                                common::ObDataBuffer &buffer);
        void handle_kill_zombie_();
        void handle_show_sessions_(common::ObPacket &pkt,
                                  common::ObNewScanner &scanner,
                                  common::ObDataBuffer &buffer);
        void handle_kill_session_(ObPacket &pkt);
        int fill_return_rows_(sql::ObPhyOperator &phy_op, common::ObNewScanner &new_scanner, sql::ObUpsResult &ups_result);
        void reset_warning_strings_();
        void fill_warning_strings_(sql::ObUpsResult &ups_result);

        int handle_write_commit_(Task &task);
        int fill_log_(Task &task, RWSessionCtx &session_ctx);
        int commit_log_();
        void try_submit_auto_freeze_();
        void log_scan_qps_();
        void log_get_qps_();
      private:
        static void phandle_non_impl(common::ObPacket &pkt, ObDataBuffer &buffer);
        static void phandle_freeze_memtable(common::ObPacket &pkt, ObDataBuffer &buffer);
        static void phandle_clear_active_memtable(common::ObPacket &pkt, ObDataBuffer &buffer);
        static void phandle_check_cur_version(common::ObPacket &pkt, ObDataBuffer &buffer);
        static void phandle_check_sstable_checksum(ObPacket &pkt, ObDataBuffer &buffer);
      private:
        static bool thandle_non_impl(TransExecutor &host, Task &task, TransParamData &pdata);
        static bool thandle_commit_end(TransExecutor &host, Task &task, TransParamData &pdata);
        static bool thandle_scan_trans(TransExecutor &host, Task &task, TransParamData &pdata);
        static bool thandle_get_trans(TransExecutor &host, Task &task, TransParamData &pdata);
        static bool thandle_write_trans(TransExecutor &host, Task &task, TransParamData &pdata);
        static bool thandle_start_session(TransExecutor &host, Task &task, TransParamData &pdata);
        static bool thandle_kill_zombie(TransExecutor &host, Task &task, TransParamData &pdata);
        static bool thandle_show_sessions(TransExecutor &host, Task &task, TransParamData &pdata);
        static bool thandle_kill_session(TransExecutor &host, Task &task, TransParamData &pdata);
        static bool thandle_end_session(TransExecutor &host, Task &task, TransParamData &pdata);
      private:
        static bool chandle_non_impl(TransExecutor &host, Task &task, CommitParamData &pdata);
        static bool chandle_write_commit(TransExecutor &host, Task &task, CommitParamData &data);
        static bool chandle_fake_write_for_keep_alive(TransExecutor &host, Task &task, CommitParamData &pdata);
        static bool chandle_send_log(TransExecutor &host, Task &task, CommitParamData &pdata);
        static bool chandle_slave_reg(TransExecutor &host, Task &task, CommitParamData &pdata);
        static bool chandle_switch_schema(TransExecutor &host, Task &task, CommitParamData &pdata);
        static bool chandle_force_fetch_schema(TransExecutor &host, Task &task, CommitParamData &pdata);
        static bool chandle_switch_commit_log(TransExecutor &host, Task &task, CommitParamData &pdata);
        static bool chandle_nop(TransExecutor &host, Task &task, CommitParamData &pdata);
      private:
        ObUtilInterface &ui_;
        common::ThreadSpecificBuffer my_thread_buffer_;
        packet_handler_pt packet_handler_[common::OB_PACKET_NUM];
        trans_handler_pt trans_handler_[common::OB_PACKET_NUM];
        commit_handler_pt commit_handler_[common::OB_PACKET_NUM];
        ObTicketQueue flush_queue_;

        common::FIFOAllocator allocator_;
        SessionCtxFactory session_ctx_factory_;
        SessionMgr session_mgr_;
        LockMgr lock_mgr_;
        ObSpinLock write_clog_mutex_;

        common::ObList<Task*> uncommited_session_list_;
        char ups_result_memory_[OB_MAX_PACKET_LENGTH];
        common::ObDataBuffer ups_result_buffer_;
        Task nop_task_;

        common::ObFIFOStream fifo_stream_;
        // add by guojinwei [log synchronize][multi_cluster] 20151028:b
        int64_t message_residence_time_us_;         ///< used for log synchronization
        int64_t message_residence_protection_us_;   ///< used for log synchronization
        int64_t message_residence_max_us_;          ///< used for log synchronization
        int64_t last_commit_log_time_us_;           ///< used for log synchronization
        // add:e
    };
  }
}

#endif //OCEANBASE_UPDATESERVER_TRANS_EXECUTOR_H_
