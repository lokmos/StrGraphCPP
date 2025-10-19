#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>

#include "strgraph/strgraph.h"
#include "strgraph/core_ops.h"
#include "strgraph/operation_registry.h"

namespace py = pybind11;

PYBIND11_MODULE(strgraph_cpp, m) {
    m.doc() = "StrGraphCPP - High-performance string computation graph backend";
    
    strgraph::core_ops::register_all();
    
    m.def("execute", 
        [](const std::string& json_data) {
            return strgraph::execute(json_data);
        },
        py::arg("json_data")
    );
    
    m.def("execute", 
        [](const std::string& json_data, const std::unordered_map<std::string, std::string>& feed_dict) {
            return strgraph::execute(json_data, feed_dict);
        },
        py::arg("json_data"),
        py::arg("feed_dict")
    );
    
    m.def("register_python_operation",
        [](const std::string& name, py::object py_func) {
            auto& registry = strgraph::OperationRegistry::get_instance();
            
            PyObject* func_ptr = py_func.ptr();
            Py_INCREF(func_ptr);
            
            auto cpp_wrapper = [func_ptr, name](
                std::span<const std::string_view> inputs,
                std::span<const std::string_view> constants
            ) -> strgraph::OpResult {
                if (!Py_IsInitialized()) {
                    throw std::runtime_error("Python interpreter is not initialized");
                }
                
                py::gil_scoped_acquire acquire;
                
                try {
                    py::object py_func = py::reinterpret_borrow<py::object>(func_ptr);
                    
                    py::list py_inputs;
                    for (const auto& sv : inputs) {
                        py_inputs.append(py::str(sv.data(), sv.size()));
                    }
                    
                    py::list py_constants;
                    for (const auto& sv : constants) {
                        py_constants.append(py::str(sv.data(), sv.size()));
                    }
                    
                    py::object result = py_func(py_inputs, py_constants);
                    
                    if (py::isinstance<py::str>(result)) {
                        return result.cast<std::string>();
                    } else if (py::isinstance<py::list>(result)) {
                        auto result_list = result.cast<py::list>();
                        std::vector<std::string> outputs;
                        outputs.reserve(result_list.size());
                        for (const auto& item : result_list) {
                            outputs.push_back(item.cast<std::string>());
                        }
                        return outputs;
                    } else {
                        throw std::runtime_error(
                            "Python custom operation must return str or List[str]"
                        );
                    }
                } catch (const py::error_already_set& e) {
                    throw std::runtime_error(
                        std::string("Python custom operation '") + name + "' failed: " + e.what()
                    );
                }
            };
            
            registry.register_op(name, cpp_wrapper);
        },
        py::arg("name"),
        py::arg("func")
    );
    
    m.attr("__version__") = "0.5.0";
}
