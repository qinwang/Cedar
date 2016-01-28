/**
* Copyright (C) 2013-2015 ECNU_DaSE.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* version 2 as published by the Free Software Foundation.
*
* @file ob_procedure_execute_stmt.h
* @brief this class present a procedure "execute" logic plan in oceanbase
*
* Created by zhujun: support procedure
*
* @version __DaSE_VERSION
* @author zhujun <51141500091@ecnu.edu.cn>
* @date 2014_11_23
*/
#ifndef OCEANBASE_SQL_OB_PROCEDURE_EXECUTE_STMT_H_
#define OCEANBASE_SQL_OB_PROCEDURE_EXECUTE_STMT_H_
#include "common/ob_string.h"
#include "common/ob_string_buf.h"
#include "common/ob_array.h"
#include "ob_basic_stmt.h"
#include "parse_node.h"
#include <map>
using namespace oceanbase::common;

namespace oceanbase {
namespace sql {

/**
 * @brief The ObProcedureExecuteStmt class
 */
class ObProcedureExecuteStmt: public ObBasicStmt {
	public:
	ObProcedureExecuteStmt() :
				ObBasicStmt(T_PROCEDURE_EXEC) {
			proc_stmt_id_=common::OB_INVALID_ID;
		}
		virtual ~ObProcedureExecuteStmt() {
		}

        /**
         * @brief set procedure name
         * @param proc_name
         * @return
         */
		int set_proc_name(ObString &proc_name);

        /**
         * @brief set procedure statement id
         * @param proc_stmt_id
         * @return
         */
		int set_proc_stmt_id(uint64_t& proc_stmt_id);

        /**
         * @brief get procedure name
         * @return
         */
        ObString& get_proc_name();
        /**
         * @brief get proccedure statement id
         * @return
         */
        uint64_t& get_proc_stmt_id();

		virtual void print(FILE* fp, int32_t level, int32_t index);

        /**
         * @brief add variable
         * @param name
         * @return
         */
		int add_variable_name(ObString& name);
        /**
         * @brief get variable name by index
         * @param index
         * @return
         */
		ObString& get_variable_name(int64_t index);
        /**
         * @brief get variable size
         * @return
         */
		int64_t get_variable_size();

        /**
         * @brief add execute parameter expression id
         * @param expr_id
         * @return
         */
		int add_param_expr(uint64_t& expr_id);

        /**
         * @brief get parameter expression by index
         * @param index
         * @return
         */
		uint64_t get_param_expr(int64_t index);
        /**
         * @brief get parameter size
         * @return
         */
		int64_t get_param_size();

	private:
        ObString proc_name_;///< procedure name
        common::ObArray<common::ObString> variable_names_;///> variables name
        common::ObArray<uint64_t> param_list_;///> parameter list
        uint64_t proc_stmt_id_;///> procedure statement id
	};


}
}

#endif
