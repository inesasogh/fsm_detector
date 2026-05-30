#include <iostream>
#include <vector>
#include <string>
#include <algorithm>


#include "Surelog/API/Surelog.h"
#include "uhdm/Serializer.h"
#include "uhdm/ElaboratorListener.h"
#include "Surelog/Design/Design.h"
#include "uhdm/uhdm.h"
#include "uhdm/vpi_user.h"

int main(){
    UHDM::Serializer serialize;
    std::vector<vpiHandle> designs = serialize.Restore("surelog.uhdm");
    if(designs.empty()){
        std::cerr<<"can not the file"<<std::endl;
        return 1;
    }
    vpiHandle top_design = designs[0];
    vpiHandle module_iter = vpi_iterate(UHDM::uhdmallModules, top_design);
    std::string assignment_var;
    std::string case_var;
    if(module_iter != nullptr){
        while(vpiHandle current_m = vpi_scan(module_iter)){
            std::string module_name = vpi_get_str(vpiName, current_m);
            std::cout << "Module name is " << module_name << "\n";
            vpiHandle process_iter = vpi_iterate(vpiProcess, current_m);
            if(process_iter != nullptr){
                while(vpiHandle current_p = vpi_scan(process_iter)){
                    PLI_INT32 type_p = vpi_get(vpiType, current_p);
                    if(type_p == vpiAlways){
                        std::cout<<"always block found"<<std::endl;
                        vpiHandle stmt = vpi_handle(vpiStmt, current_p);
                        if(stmt != nullptr){
                            PLI_INT32 type_stmt = vpi_get(vpiType, stmt);
                            if (type_stmt == vpiEventControl){
                                vpiHandle iner_stmt = vpi_handle(vpiStmt, stmt);
                                if(iner_stmt != nullptr){
                                    PLI_INT32 type_iner_stmt = vpi_get(vpiType,iner_stmt);
                                    if(type_iner_stmt == vpiBegin){
                                        vpiHandle sub_iner_stmt = vpi_iterate(vpiStmt, iner_stmt);
                                        if(sub_iner_stmt != nullptr){
                                            while(vpiHandle sub_stmt = vpi_scan(sub_iner_stmt)){
                                                PLI_INT32 type_sub = vpi_get(vpiType ,sub_stmt);
                                                if(type_sub == vpiAssignment){
                                                    std::cout<<"Found state reg update assignment"<<std::endl;
                                                    vpiHandle LHS = vpi_handle(vpiLhs,sub_stmt);
                                                    vpiHandle RHS = vpi_handle(vpiRhs, sub_stmt);
                                                    if(LHS != nullptr){
                                                        assignment_var = vpi_get_str(vpiName,LHS);
                                                        std::cout<<"LHS:"<<vpi_get_str(vpiName, LHS);
                                                    }if(RHS != nullptr){
                                                        std::cout<<"RHS:"<<vpi_get_str(vpiName, RHS);
                                                    }                                                }
                                                if(type_sub == vpiCase){
                                                    std::cout<<"Found Case statement inside clocked block" <<std::endl;
                                                }
                                            }
                                        }
                                    }
                                    vpi_free_object(iner_stmt);
                                }
                            }
                            else if(type_stmt == vpiBegin){
                                vpiHandle block_stmt = vpi_iterate(vpiStmt, stmt);
                                if(block_stmt != nullptr){
                                    while (vpiHandle sub_stmt = vpi_scan(block_stmt))
                                    {
                                        PLI_INT32 type_sub_stmt = vpi_get(vpiType,sub_stmt);
                                        if(type_sub_stmt == vpiCase){
                                            std::cout <<" FSM Case statement found"<<std::endl;
                                        }
                                        
                                    }
                                    
                                }
                            }
                            else if(type_stmt == vpiCase){
                               std::cout<<"Found FSM Case statement"<<std::endl;
                               vpiHandle case_expr = vpi_handle(vpiCondition, stmt);
                               if(case_expr != nullptr){
                                case_var = vpi_get_str(vpiName,case_expr);
                                std::cout<<"Tracking variable:" <<vpi_get_str(vpiName, case_expr);
                                vpi_free_object(case_expr);
                               }
                               vpiHandle case_item_iter = vpi_iterate(vpiCaseItem, stmt);
                               if(case_item_iter != nullptr){
                                while(vpiHandle case_item = vpi_scan(case_item_iter)){
                                    vpiHandle expr_iter = vpi_iterate(vpiExpr, case_item);
                                    if(expr_iter != nullptr){
                                        while(vpiHandle expr = vpi_scan(expr_iter)){
                                            std::cout<< "state"<<vpi_get_str(vpiName,expr)<<std::endl;
                                        }
                                    }
                                    else{
                                        std::cout<<"stste:default branch"<<std::endl;
                                    }
                                }
                               }
                            }
                            std::cout<<"statment type inside always block" << type_stmt << std::endl;
                            vpi_free_object(stmt);
                        }
                    }
                }
                if(!assignment_var.empty() && !case_var.empty() && assignment_var == case_var){
                    std::cout<<"FSM Detected"<<std::endl;
                    std::cout<<"state variable:"<<assignment_var<<std::endl;
                }
            }
            else{
                std::cout<<"Process not found" << std::endl;
            }
        }
    }
    else{
        std::cout<<"No instances found "<<std::endl;
    }  
}