/*
 * Copyright 2002-2014 the original author or authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      CC/LICENSE
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <sstream>
#include <json/json.h>
#include "node_jsonget_array.h"

NodeJsonGetArray::NodeJsonGetArray(const uint32_t id, const string& name,
                                   const string& type, const string& desc, const key_map_t& keymap)
    : NodeBase(id, name, type, desc, keymap) {
}

NodeJsonGetArray::~NodeJsonGetArray(void) {
}

NodeBase* NodeJsonGetArray::run(base_script_t* param) {
    IVR_TRACE("%s", enter(param->name_var_map).c_str());

    const char* exit = EXIT_FAIL;
    std::string value = "";
    std::string valueindex = "";
    std::string input = "";

    variable_t var_value;
    variable_t var_reason;
    std::string reason_type_error = "reason type error";
    var_value.pvalue = &reason_type_error;
    var_reason.pvalue = &reason_type_error;

    if (param->name_var_map.find(_value) == param->name_var_map.end()) {
        IVR_WARN("not found variable %s", _value.c_str());
        goto LEAVE;
    }

    if (param->name_var_map.find(_reason) == param->name_var_map.end()) {
        IVR_WARN("not found variable %s", _reason.c_str());
        goto LEAVE;
    }

    var_value = param->name_var_map[_value];
    var_reason = param->name_var_map[_reason];

    if (var_reason.type != INT32) {
        IVR_WARN("变量%s 的类型错误", _reason.c_str());
        //(*(std::string*)var_reason.pvalue).assign("5");
        goto LEAVE;
    }

    (*(std::string*)var_reason.pvalue).assign("0");

    if (parse_all(_input, param->name_var_map, input)
            && parse_expression(_valueindex, param->name_var_map, valueindex)) {
        IVR_TRACE("INPUT: %s;", input.c_str());
        //获取json串
        json_object* obj = json_tokener_parse(input.c_str());

        if (is_error(obj) || NULL == obj) {
            IVR_TRACE("未找到需要赋值的变量");
            (*(std::string*)var_reason.pvalue).assign("1");
            goto LEAVE;
        }

        const char* tmp = json_object_get_string(obj);

        if (tmp[0] >= '0' && tmp[0] <= '9') {
            IVR_TRACE("未找到需要赋值的变量");
            (*(std::string*)var_reason.pvalue).assign("1");
            json_object_put(obj);
            goto LEAVE;
        }

        //检查是否为array
        if (! json_object_is_type(obj, json_type_array)) {
            IVR_WARN("jsongetarray中出现非array值");
            //TODO: assign what?
            (*(std::string*)var_reason.pvalue).assign("10");
            json_object_put(obj);
            //json_object_put(obj_value);
            goto LEAVE;
        }

        if (!ivr_tools_t::is_nonnegative_integer(valueindex)) {
            IVR_WARN("下标需为非负整数值: %s", valueindex.c_str());
            //TODO: assign what?
            (*(std::string*)var_reason.pvalue).assign("11");
            json_object_put(obj);
            //json_object_put(obj_value);
            goto LEAVE;
        }

        int32_t tmp_value_index = atoi(valueindex.c_str());

        int32_t array_len = json_object_array_length(obj);

        if (tmp_value_index < 0 || tmp_value_index >= array_len) {
            IVR_WARN("jsonget array 中下标值越界");
            //TODO: assign what?
            (*(std::string*)var_reason.pvalue).assign("12");
            json_object_put(obj);
            //json_object_put(obj_value);
            goto LEAVE;
        }

        json_object* obj_value = json_object_array_get_idx(obj, tmp_value_index);

        if (is_error(obj_value) || NULL == obj_value) {
            IVR_TRACE("未找到下标为%d的元素", tmp_value_index);
            //TODO: assign what?
            (*(std::string*)var_reason.pvalue).assign("13");
            json_object_put(obj);
            //json_object_put(obj_value);
            goto LEAVE;
        }

        json_type jtype = json_object_get_type(obj_value);

        if (0 == strcasecmp(_valuetype.c_str(), PARAMITEM_TYPE_STRING)) {
            if (jtype != json_type_string && jtype != json_type_object
                    && jtype != json_type_array) {
                IVR_TRACE("json值类型错误，非string、object、array类型");
                (*(std::string*)var_reason.pvalue).assign("5");
                json_object_put(obj);
                goto LEAVE;
            }

            const char* cstr_value = json_object_get_string(obj_value);

            if (NULL == cstr_value) {
                IVR_TRACE("转换数组值错误");
                (*(std::string*)var_reason.pvalue).assign("3");
                json_object_put(obj);
                goto LEAVE;
            }

            if (var_value.type != STRING) {
                IVR_TRACE("赋值变量类型错误");
                (*(std::string*)var_reason.pvalue).assign("4");
                json_object_put(obj);
                goto LEAVE;
            }

            value.assign(cstr_value);
            *(string*)var_value.pvalue = value;
            exit = EXIT_SUCC;
            json_object_put(obj);
            goto LEAVE;
        } else if (0 == strcasecmp(_valuetype.c_str(), PARAMITEM_TYPE_INT32)) {
            if (jtype != json_type_int) {
                IVR_TRACE("json值类型错误，非int类型");
                (*(std::string*)var_reason.pvalue).assign("5");
                json_object_put(obj);
                goto LEAVE;
            }

            int32_t int32_value = json_object_get_int(obj_value);

            if (var_value.type != INT32) {
                IVR_TRACE("赋值变量类型错误");
                (*(std::string*)var_reason.pvalue).assign("4");
                json_object_put(obj);
                goto LEAVE;
            }

            std::stringstream ss;
            ss << int32_value;
            ss >> value;
            *(string*)var_value.pvalue = value;
            exit = EXIT_SUCC;
            json_object_put(obj);
            goto LEAVE;
        } else if (0 == strcasecmp(_valuetype.c_str(), PARAMITEM_TYPE_BOOL)) {
            if (jtype != json_type_boolean) {
                IVR_TRACE("json值类型错误，非boolean类型");
                (*(std::string*)var_reason.pvalue).assign("5");
                json_object_put(obj);
                goto LEAVE;
            }

            bool bool_value = json_object_get_boolean(obj_value);

            if (var_value.type != INT32) {
                IVR_TRACE("赋值变量类型错误");
                (*(std::string*)var_reason.pvalue).assign("4");
                json_object_put(obj);
                goto LEAVE;
            }

            if (bool_value) {
                value.assign("1");
            } else {
                value.assign("0");
            }

            *(string*)var_value.pvalue = value;
            exit = EXIT_SUCC;
            json_object_put(obj);
            goto LEAVE;
        } else {
            IVR_TRACE("Param valuetype is invalid: %s", _valuetype.c_str());
            (*(std::string*)var_reason.pvalue).assign("3");
            json_object_put(obj);
            goto LEAVE;
        }
    }

LEAVE:
    IVR_TRACE("%s: Input:%s,ValueType:%s,Value:%s,Reason:%s exit from %s(%s)",
              leave(param->name_var_map).c_str(),
              input.c_str(), _valuetype.c_str(), value.c_str(),
              (*(std::string*)var_reason.pvalue).c_str(), exit, value.c_str());
    return _exit_node_map[exit];
}

bool NodeJsonGetArray::load_other() {
    return NodeBase::load_other()
           && valid_str(_key_map, PARAM_INPUT, _input)
           && valid_str(_key_map, PARAM_VALUEINDEX, _valueindex)
           && valid_str(_key_map, PARAM_VALUETYPE, _valuetype)
           && valid_str(_key_map, PARAM_VALUE, _value)
           && valid_str(_key_map, PARAM_REASON, _reason);
}

std::string NodeJsonGetArray::enter(const map<string, variable_t>& vars)const {
    ostringstream ostm;
    ostm << NodeBase::enter(vars);
    ostm << "Input:" << _input << ",ValueType:" << _valuetype
         << ",Value:" << _value << ",Reason:" << _reason;
    return ostm.str();
}

std::string NodeJsonGetArray::leave(const map<string, variable_t>& vars)const {
    ostringstream ostm;
    ostm << NodeBase::leave(vars);
    return ostm.str();
}

const char* NodeJsonGetArray::PARAM_INPUT = "input";
const char* NodeJsonGetArray::PARAM_VALUEINDEX = "index";
const char* NodeJsonGetArray::PARAM_VALUETYPE = "valuetype";
const char* NodeJsonGetArray::PARAM_VALUE = "value";
const char* NodeJsonGetArray::PARAM_REASON = "reason";

const char* NodeJsonGetArray::PARAMITEM_TYPE_STRING = "string";
const char* NodeJsonGetArray::PARAMITEM_TYPE_INT32 = "int32";
const char* NodeJsonGetArray::PARAMITEM_TYPE_BOOL = "bool";
