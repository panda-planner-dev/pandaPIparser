#include "englishTranscriptionWriter.hpp"
#include "parsetree.hpp" // Defines parsed_task, parsed_method, general_formula, var_declaration, etc.
#include "domain.hpp"    // Defines task, method, literal, AND global collections like parsed_abstract, parsed_primitive, parsed_methods, primitive_tasks, method_precondition_action_name etc.
#include <vector>
#include <string>
#include <map>
#include <set>
#include <sstream>     // Also included in hddl_helpers.cpp, good practice for direct dependencies
#include <algorithm>   // For std::sort, std::remove_if etc.
#include <cctype>      // For isdigit

// Content from ./tmp/hddl_helpers.cpp will be appended next.
// The static functions from hddl_helpers.cpp will be effectively pasted here.

// --- Start of HDDL Reconstruction Helpers ---
#include <sstream> // Ensure sstream is included here if not globally yet

static std::string format_parameters_hddl(const var_declaration* params_decl) {
    if (!params_decl || params_decl->vars.empty()) {
        return "";
    }
    std::ostringstream oss;
    bool first = true;
    for (const auto& var_pair : params_decl->vars) {
        if (!first) {
            oss << " ";
        }
        oss << var_pair.first;
        if (!var_pair.second.empty() && var_pair.second != "object") {
            oss << " - " << var_pair.second;
        }
        first = false;
    }
    return oss.str();
}

static std::string format_general_formula_pddl(const general_formula* formula);

static std::string format_atom_pddl(const general_formula* formula) {
    std::ostringstream oss;
    oss << "(" << formula->predicate;
    for (const std::string& arg : formula->arguments.vars) {
        oss << " " << arg;
    }
    oss << ")";
    return oss.str();
}

static std::string format_general_formula_pddl(const general_formula* formula) {
    if (!formula || formula->type == EMPTY) {
        return "";
    }
    std::ostringstream oss;
    switch (formula->type) {
        case ATOM:
            oss << format_atom_pddl(formula);
            break;
        case NOTATOM:
            oss << "(not " << format_atom_pddl(formula) << ")";
            break;
        case AND:
            oss << "(and";
            for (const auto* sub : formula->subformulae) {
                std::string sub_str = format_general_formula_pddl(sub);
                if (!sub_str.empty()) oss << "\n            " << sub_str;
            }
            oss << "\n        )";
            break;
        case OR:
            oss << "(or";
            for (const auto* sub : formula->subformulae) {
                 std::string sub_str = format_general_formula_pddl(sub);
                if (!sub_str.empty()) oss << "\n            " << sub_str;
            }
            oss << "\n        )";
            break;
        case EQUAL:
            oss << "(= " << formula->arg1 << " " << formula->arg2 << ")";
            break;
        case NOTEQUAL:
            oss << "(not (= " << formula->arg1 << " " << formula->arg2 << "))";
            break;
        case FORALL:
            oss << "(forall (" << format_parameters_hddl(&formula->qvariables) << ")";
            if (!formula->subformulae.empty()) {
                oss << "\n            " << format_general_formula_pddl(formula->subformulae[0]);
            }
            oss << "\n        )";
            break;
        case EXISTS:
            oss << "(exists (" << format_parameters_hddl(&formula->qvariables) << ")";
            if (!formula->subformulae.empty()) {
                oss << "\n            " << format_general_formula_pddl(formula->subformulae[0]);
            }
            oss << "\n        )";
            break;
        case WHEN:
            oss << "(when ";
            if (!formula->subformulae.empty()){ oss << format_general_formula_pddl(formula->subformulae[0]);}
            oss << "\n            ";
            if (formula->subformulae.size() > 1){ oss << format_general_formula_pddl(formula->subformulae[1]);}
            oss << "\n        )";
            break;
        default:
            oss << "[UnknownOrUnsupportedPDDLFormulaType: " << formula->type << "]";
    }
    return oss.str();
}

static std::string reconstruct_hddl_task(const parsed_task* task) {
    if (!task) return "";
    std::ostringstream oss;
    oss << "(:task " << task->name;
    std::string params_str = format_parameters_hddl(task->arguments);
    if (!params_str.empty()) {
        oss << " :parameters (" << params_str << ")";
    }
    oss << ")";
    return oss.str();
}

static std::string reconstruct_hddl_subtasks(const parsed_task_network* tn, bool is_ordered_block) {
    if (!tn || tn->tasks.empty()) {
        return "()";
    }
    std::ostringstream oss;
    bool wrap_in_and = tn->tasks.size() > 1;

    if (wrap_in_and) oss << "(and";

    for (const auto* sub_task_ptr : tn->tasks) {
        if (!sub_task_ptr) continue;

        if (wrap_in_and) oss << "\n            ";
        else if (is_ordered_block && tn->tasks.size() == 1) oss << "\n        ";
        else if (!is_ordered_block && tn->tasks.size() == 1) oss << " ";

        oss << "(";
        if (!sub_task_ptr->id.empty() && sub_task_ptr->id != sub_task_ptr->task &&
            sub_task_ptr->id.find("method_precondition_action") == std::string::npos) {
            bool is_numeric_id = true;
            for(char c : sub_task_ptr->id) { if (!isdigit(c)) { is_numeric_id = false; break; } }
            if (!is_numeric_id) oss << sub_task_ptr->id << " ";
        }
        oss << sub_task_ptr->task;
        if (sub_task_ptr->arguments && !sub_task_ptr->arguments->vars.empty()) {
            for (const auto& arg : sub_task_ptr->arguments->vars) {
                oss << " " << arg;
            }
        }
        oss << ")";
    }

    if (wrap_in_and) oss << "\n        )";

    return oss.str();
}

static std::string reconstruct_hddl_ordering(const std::vector<std::pair<std::string, std::string>*>& ordering) {
    if (ordering.empty()) return "";
    std::ostringstream oss;
    oss << " :ordering (and";
    for (const auto* order_pair_ptr : ordering) {
        if (!order_pair_ptr) continue;
        oss << " (< " << order_pair_ptr->first << " " << order_pair_ptr->second << ")";
    }
    oss << ")";
    return oss.str();
}

static std::string reconstruct_hddl_method(const parsed_method* method, const std::string& decomposed_task_name) {
    if (!method) return "";
    std::ostringstream oss;
    oss << "(:method " << method->name;
    std::string params_str = format_parameters_hddl(method->vars);
    if (!params_str.empty()) oss << "\n    :parameters (" << params_str << ")";
    oss << "\n    :task (" << decomposed_task_name;
    for (const auto& arg : method->atArguments) oss << " " << arg;
    oss << ")";
    std::string prec_str = (method->prec && method->prec->type != EMPTY) ? format_general_formula_pddl(method->prec) : "";
    if (!prec_str.empty()) oss << "\n    :precondition " << prec_str;

    if (method->tn) {
        bool has_subtasks = !method->tn->tasks.empty();
        bool use_ordered_keyword = false;
        if (has_subtasks && !method->tn->ordering.empty() && (method->tn->constraint == nullptr || method->tn->constraint->type == EMPTY)) {
            use_ordered_keyword = true;
        }

        if (has_subtasks) {
            if (use_ordered_keyword) {
                 oss << "\n    :ordered-subtasks " << reconstruct_hddl_subtasks(method->tn, true);
            } else {
                 oss << "\n    :subtasks " << reconstruct_hddl_subtasks(method->tn, false);
                 if (!method->tn->ordering.empty()) {
                    oss << "\n   " << reconstruct_hddl_ordering(method->tn->ordering);
                 }
            }
        } else {
             oss << "\n    :subtasks ()";
        }
        std::string constraints_str = (method->tn->constraint && method->tn->constraint->type != EMPTY) ? format_general_formula_pddl(method->tn->constraint) : "";
        if (!constraints_str.empty()) oss << "\n    :constraints " << constraints_str;
    } else {
        oss << "\n    :subtasks ()";
    }
    oss << "\n)";
    return oss.str();
}
// --- End of HDDL Reconstruction Helpers ---

// --- Start of English Transcription Helpers ---
// Note: Includes for sstream, vector, string, map, set are at the top of the file.
// Forward declarations are also not needed here as definitions are provided.

// Assumes parsetree.hpp and domain.hpp are included in the main .cpp file where these helpers will reside.

static const var_declaration* get_task_parameters_by_name(const std::string& task_name, const std::map<std::string, const parsed_task*>& all_tasks_map) {
    auto it = all_tasks_map.find(task_name);
    return (it != all_tasks_map.end() && it->second) ? it->second->arguments : nullptr;
}

// --- English Formatting Refinements (from ./tmp/english_format_refinement.cpp) ---
static std::string format_english_parameters(
    const var_declaration* params_decl,
    const std::vector<std::string>* param_names_override,
    const std::map<std::string, const parsed_task*>* all_tasks_map_for_types,
    const std::string& task_context_for_types,
    int* out_param_count = nullptr) { // Added out_param_count

    std::vector<std::pair<std::string, std::string>> effective_params;
    if (param_names_override) {
        const var_declaration* original_task_params = nullptr;
        if (all_tasks_map_for_types && !task_context_for_types.empty()){
            original_task_params = get_task_parameters_by_name(task_context_for_types, *all_tasks_map_for_types);
        }
        for (size_t i = 0; i < param_names_override->size(); ++i) {
            std::string type = "object";
            if (original_task_params && i < original_task_params->vars.size()) type = original_task_params->vars[i].second;
            effective_params.push_back({param_names_override->at(i), type});
        }
    } else if (params_decl) {
        effective_params = params_decl->vars;
    }

    if (out_param_count) *out_param_count = effective_params.size();

    if (effective_params.empty()) return "no parameters";

    std::ostringstream oss;
    for (size_t i = 0; i < effective_params.size(); ++i) {
        const auto& var_pair = effective_params[i];
        std::string type_str = var_pair.second;
        std::string name_str = var_pair.first;

        if (type_str == "object" || type_str.empty()) {
            oss << name_str;
        } else {
            std::string article = (!type_str.empty() && std::string("aeiouAEIOU").find(type_str[0]) != std::string::npos) ? "an " : "a ";
            oss << article << type_str << " " << name_str;
        }
        if (effective_params.size() > 1) { // Only add commas/ands if more than one param
            if (i < effective_params.size() - 2) oss << ", ";
            else if (i == effective_params.size() - 2) oss << " and ";
        }
    }
    return oss.str();
}

static std::string format_english_subtask(const sub_task* st, const std::map<std::string, const parsed_task*>& all_tasks_map, const std::set<std::string>& action_names) {
    std::ostringstream oss;
    oss << st->task; // Subtask name

    std::vector<std::pair<std::string, std::string>> subtask_effective_params;
    bool is_action = action_names.count(st->task) > 0;

    if (st->arguments && !st->arguments->vars.empty()) {
        const var_declaration* original_task_params = nullptr;
        if (!is_action) {
             original_task_params = get_task_parameters_by_name(st->task, all_tasks_map);
        }
        for (size_t i = 0; i < st->arguments->vars.size(); ++i) {
            std::string type = "object";
            if (original_task_params && i < original_task_params->vars.size()) {
                type = original_task_params->vars[i].second;
            }
            subtask_effective_params.push_back({st->arguments->vars[i], type});
        }
    }

    if (subtask_effective_params.empty()) {
        oss << " with no parameters"; // Simplified: always state if no params in call
    } else {
        oss << " with ";
        for (size_t i = 0; i < subtask_effective_params.size(); ++i) {
            const auto& var_pair = subtask_effective_params[i];
            std::string type_str = var_pair.second;
            if (is_action || type_str == "object" || type_str.empty()) {
                 oss << var_pair.first;
            } else {
                std::string article = (!type_str.empty() && std::string("aeiouAEIOU").find(type_str[0]) != std::string::npos) ? "an " : "a ";
                oss << article << type_str << " " << var_pair.first;
            }
            if (i < subtask_effective_params.size() - 2) oss << ", ";
            else if (i == subtask_effective_params.size() - 2) oss << " and ";
        }
        oss << " as parameter(s)";
    }
    return oss.str();
}
// End of functions from /tmp/english_format_refinement.cpp that were part of "English Transcription Helpers"

// Helper to determine the sequence of subtasks for English phrasing (Unchanged from original English Helpers)
static std::vector<const sub_task*> get_ordered_subtasks(const parsed_method* method) {
    std::vector<const sub_task*> result_list;
    if (!method || !method->tn || method->tn->tasks.empty()) {
        return result_list; // Return empty list
    }

    const auto& sub_tasks = method->tn->tasks;
    const auto& ordering_constraints = method->tn->ordering;
    int num_subtasks = sub_tasks.size();

    if (ordering_constraints.empty()) {
        for (const auto* st_ptr : sub_tasks) {
            if(st_ptr) result_list.push_back(st_ptr);
        }
        return result_list;
    }

    std::map<std::string, const sub_task*> task_id_to_ptr_map;
    for (const auto* st_ptr : sub_tasks) {
        if(st_ptr) task_id_to_ptr_map[st_ptr->id] = st_ptr;
    }

    std::map<std::string, std::string> successor_map;
    std::map<std::string, int> in_degree;
    std::set<std::string> all_task_ids_in_ordering;

    for (const auto* st_ptr : sub_tasks) {
        if(st_ptr) in_degree[st_ptr->id] = 0;
    }

    for (const auto* order_pair : ordering_constraints) {
        if (order_pair && task_id_to_ptr_map.count(order_pair->first) && task_id_to_ptr_map.count(order_pair->second)) {
            successor_map[order_pair->first] = order_pair->second;
            in_degree[order_pair->second]++;
            all_task_ids_in_ordering.insert(order_pair->first);
            all_task_ids_in_ordering.insert(order_pair->second);
        }
    }

    std::vector<std::string> queue;
    for (const auto* st_ptr : sub_tasks) {
        if (st_ptr && in_degree[st_ptr->id] == 0) {
            queue.push_back(st_ptr->id);
        }
    }

    if (queue.empty()) {
        for (const auto* st_ptr : sub_tasks) {
            if (st_ptr && all_task_ids_in_ordering.count(st_ptr->id) && in_degree[st_ptr->id] == 0) {
            } else if (st_ptr && !all_task_ids_in_ordering.count(st_ptr->id)) {
                queue.push_back(st_ptr->id);
            }
        }
    }

    std::set<std::string> visited_in_current_path;

    int head = 0;
    while(head < queue.size()){
        std::string current_task_id = queue[head++];
        if(task_id_to_ptr_map.count(current_task_id)){
             result_list.push_back(task_id_to_ptr_map[current_task_id]);
        }
        auto successor_iter = successor_map.find(current_task_id);
        if (successor_iter != successor_map.end()) {
            std::string next_task_id = successor_iter->second;
            if (task_id_to_ptr_map.count(next_task_id)) {
                 in_degree[next_task_id]--;
                 if (in_degree[next_task_id] == 0) {
                    bool already_in_queue = false;
                    for(int i=head; i < queue.size(); ++i) if(queue[i] == next_task_id) already_in_queue = true;
                    if(!already_in_queue) {
                        bool already_processed = false;
                        for(const auto* rt : result_list) if(rt->id == next_task_id) already_processed = true;
                        if(!already_processed) queue.push_back(next_task_id);
                    }
                 }
            }
        }
    }

    if (result_list.size() != (size_t)num_subtasks) {
        std::set<std::string> processed_ids;
        for(const auto* st : result_list) processed_ids.insert(st->id);

        for (const auto* st_ptr : sub_tasks) {
            if (st_ptr && processed_ids.find(st_ptr->id) == processed_ids.end()) {
                result_list.push_back(st_ptr);
            }
        }
    }
    return result_list;
}
// --- End of English Transcription Helpers (Original parts + Refined parts) ---

// --- Start of Main English Generation Functions (Refined) ---
// Note: Includes for sstream, vector, string, map, set, algorithm are at the top of the file.
static std::string generate_english_for_task(const parsed_task* task, const std::map<std::string, const parsed_task*>& all_tasks_map) {
    if (!task) return "Error: Null task provided.";
    std::ostringstream oss;
    oss << "There is a task " << task->name;
    int param_count = 0;
    std::string params_str = format_english_parameters(task->arguments, nullptr, &all_tasks_map, task->name, &param_count);
    if (param_count == 0) {
        oss << " that takes no parameters.";
    } else {
        oss << " that takes " << params_str << (param_count == 1 ? " as a parameter." : " as parameters.");
    }
    return oss.str();
}

static std::string generate_english_for_method(
    const parsed_method* method,
    const std::string& decomposed_task_name,
    const std::map<std::string, const parsed_task*>& all_tasks_map,
    const std::set<std::string>& action_names) {

    if (!method) return "Error: Null method provided.";
    std::ostringstream oss;
    int param_count = 0;

    oss << "There is a method " << method->name;
    std::string method_params_str = format_english_parameters(method->vars, nullptr, &all_tasks_map, decomposed_task_name, &param_count);
    if (param_count == 0) {
        oss << " that takes no parameters";
    } else {
        oss << " that takes " << method_params_str << (param_count == 1 ? " as a parameter" : " as parameters");
    }

    param_count = 0;
    std::string accomplished_task_params_str = format_english_parameters(nullptr, &method->atArguments, &all_tasks_map, decomposed_task_name, &param_count);
    oss << ", and accomplishes the task " << decomposed_task_name;
    if (param_count == 0 && method->atArguments.empty()) { // Check original atArguments too
        oss << " with no parameters.";
    } else {
        std::string accomplished_params_final_str;
        if (param_count > 0) accomplished_params_final_str = accomplished_task_params_str;
        else if (!method->atArguments.empty()) {
            std::ostringstream temp_oss;
            for(size_t i=0; i<method->atArguments.size(); ++i) {
                temp_oss << method->atArguments[i];
                if (i < method->atArguments.size() -2) temp_oss << ", ";
                else if (i == method->atArguments.size() -2) temp_oss << " and ";
            }
            accomplished_params_final_str = temp_oss.str();
        }

        if (accomplished_params_final_str.empty() || accomplished_params_final_str == "no parameters"){
             oss << " with no parameters.";
        } else {
            oss << " with " << accomplished_params_final_str << (method->atArguments.size() == 1 ? " as parameter." : " as parameters.");
        }
    }

    std::string prec_str_eng = (method->prec && method->prec->type != EMPTY) ? format_general_formula_pddl(method->prec) : "";
    if (!prec_str_eng.empty()) {
        oss << " This method applies if the PDDL condition `" << prec_str_eng << "` is true.";
    }

    std::string constraint_str_eng = (method->tn && method->tn->constraint && method->tn->constraint->type != EMPTY) ? format_general_formula_pddl(method->tn->constraint) : "";
    if(!constraint_str_eng.empty()){
        oss << " The method has the constraint that `" << constraint_str_eng << "`.";
    }

    if (!method->tn || method->tn->tasks.empty()) {
        oss << " The method has no subtasks.";
    } else {
        int num_subtasks = method->tn->tasks.size();
        std::vector<const sub_task*> subtask_print_order = get_ordered_subtasks(method);
        bool use_sequential_phrasing = false;
        if (num_subtasks > 1 && !method->tn->ordering.empty()) {
            if (subtask_print_order.size() == (size_t)num_subtasks && method->tn->ordering.size() >= (size_t)num_subtasks - 1) {
                use_sequential_phrasing = true;
            }
        }
        if (num_subtasks <= 1) use_sequential_phrasing = false;

        std::string num_str;
        if (num_subtasks == 1) num_str = "one";
        else if (num_subtasks == 2) num_str = "two";
        else if (num_subtasks == 3) num_str = "three";
        else if (num_subtasks == 4) num_str = "four";
        else num_str = std::to_string(num_subtasks);

        if (use_sequential_phrasing) {
            oss << " The method has " << num_str << " sequentially ordered subtasks: ";
            std::string ordinals[] = {"first", "second", "third", "fourth", "fifth", "sixth", "seventh", "eighth", "ninth", "tenth"};
            for (size_t i = 0; i < subtask_print_order.size(); ++i) {
                if (i > 0) oss << (i == subtask_print_order.size() - 1 ? ", and finally" : ", then");
                oss << " ";
                if (i < sizeof(ordinals)/sizeof(std::string)) oss << ordinals[i]; else oss << (i+1) << "th";
                oss << " do subtask " << format_english_subtask(subtask_print_order[i], all_tasks_map, action_names);
            }
            oss << ".";
        } else if (num_subtasks == 1) {
             oss << " The method has only one subtask, " << format_english_subtask(subtask_print_order[0], all_tasks_map, action_names) << ".";
        } else {
            oss << " The method has " << num_str << " subtasks";
            if (method->tn->ordering.empty()) {
                oss << (": ");
            } else {
                oss << " which can be done in any order satisfying these constraints: ";
            }
            for (size_t i = 0; i < subtask_print_order.size(); ++i) {
                oss << format_english_subtask(subtask_print_order[i], all_tasks_map, action_names);
                if (i < subtask_print_order.size() - 1) {
                    if (subtask_print_order.size() == 2 && i == 0) oss << " and ";
                    else if (i == subtask_print_order.size() - 2) oss << ", and ";
                    else oss << ", ";
                }
            }
            oss << ".";
            if (!method->tn->ordering.empty()) {
                std::string ordering_hddl = reconstruct_hddl_ordering(method->tn->ordering);
                std::string clean_ordering_hddl = ordering_hddl;
                size_t pos = clean_ordering_hddl.find("(and");
                if (pos != std::string::npos) clean_ordering_hddl = clean_ordering_hddl.substr(pos);
                oss << " The ordering constraints are: `" << clean_ordering_hddl << "`.";
            }
        }
    }
    return oss.str();
}
// --- End of Main English Generation Functions (Refined) ---

// --- Main Writer Function (Final Implementation) ---
// Includes and other static helper functions (HDDL reconstruction, English helpers, English generators)
// are assumed to be already present in the file from previous steps.

void write_instance_as_english_transcription(std::ostream& domain_out, std::ostream& problem_out) {
    domain_out << "; English Transcription for HDDL Domain File" << std::endl;
    domain_out << "; Generated by pandaPIparser with --english-transcription option" << std::endl;
    domain_out << std::endl;

    // Build a map of all task definitions for easy lookup of parameter types
    std::map<std::string, const parsed_task*> all_tasks_map;
    for (const auto& task_def : parsed_abstract) {
        all_tasks_map[task_def.name] = &task_def;
    }
    // Also add primitive tasks to this map, as they can be subtasks or decomposed by methods.
    for (const auto& task_def : parsed_primitive) {
        all_tasks_map[task_def.name] = &task_def;
    }

    std::set<std::string> action_names;
    for(const auto& action_task : primitive_tasks) { // primitive_tasks from domain.hpp (compiled actions)
        action_names.insert(action_task.name);
    }

    // --- Transcribe Abstract Tasks ---
    if (!parsed_abstract.empty()) {
        for (const auto& task_def : parsed_abstract) {
            domain_out << "% ENGLISH_TRANSCRIPTION_START" << std::endl;
            domain_out << "% " << generate_english_for_task(&task_def, all_tasks_map) << std::endl;
            domain_out << "% ENGLISH_TRANSCRIPTION_END" << std::endl;
            domain_out << "HDDL_CONSTRUCT_START" << std::endl;
            domain_out << reconstruct_hddl_task(&task_def) << std::endl;
            domain_out << "HDDL_CONSTRUCT_END" << std::endl << std::endl;
        }
    }

    // --- Transcribe Primitive Tasks (that are not PDDL actions) ---
    if (!parsed_primitive.empty()) {
        std::vector<const parsed_task*> sorted_primitive_tasks;
        for(const auto& task_def : parsed_primitive) sorted_primitive_tasks.push_back(&task_def);
        std::sort(sorted_primitive_tasks.begin(), sorted_primitive_tasks.end(), [](const parsed_task* a, const parsed_task* b){ return a->name < b->name; });

        for (const auto* task_def_ptr : sorted_primitive_tasks) {
            if (action_names.find(task_def_ptr->name) == action_names.end()) { // If not found in compiled action names
                domain_out << "% ENGLISH_TRANSCRIPTION_START" << std::endl;
                domain_out << "% " << generate_english_for_task(task_def_ptr, all_tasks_map) << std::endl;
                domain_out << "% ENGLISH_TRANSCRIPTION_END" << std::endl;
                domain_out << "HDDL_CONSTRUCT_START" << std::endl;
                domain_out << reconstruct_hddl_task(task_def_ptr) << std::endl;
                domain_out << "HDDL_CONSTRUCT_END" << std::endl << std::endl;
            }
        }
    }

    // --- Transcribe Methods ---
    // To ensure consistent output order for methods, iterate through sorted task names first
    std::vector<std::string> sorted_decomposed_task_names;
    for(const auto& method_list_entry : parsed_methods) {
        sorted_decomposed_task_names.push_back(method_list_entry.first);
    }
    std::sort(sorted_decomposed_task_names.begin(), sorted_decomposed_task_names.end());

    if (!parsed_methods.empty()){
        for (const auto& task_name_decomposed : sorted_decomposed_task_names) {
            const auto& method_list = parsed_methods.at(task_name_decomposed);
            // Sort methods by name for this task for consistent output
            std::vector<const parsed_method*> sorted_methods_for_task;
            for(const auto& method_def : method_list) sorted_methods_for_task.push_back(&method_def);
            std::sort(sorted_methods_for_task.begin(), sorted_methods_for_task.end(), [](const parsed_method* a, const parsed_method* b){ return a->name < b->name; });

            for (const auto* method_def_ptr : sorted_methods_for_task) {
                domain_out << "% ENGLISH_TRANSCRIPTION_START" << std::endl;
                domain_out << "% " << generate_english_for_method(method_def_ptr, task_name_decomposed, all_tasks_map, action_names) << std::endl;
                domain_out << "% ENGLISH_TRANSCRIPTION_END" << std::endl;
                domain_out << "HDDL_CONSTRUCT_START" << std::endl;
                domain_out << reconstruct_hddl_method(method_def_ptr, task_name_decomposed) << std::endl;
                domain_out << "HDDL_CONSTRUCT_END" << std::endl << std::endl;
            }
        }
    }

    // --- List PDDL Actions --- (Already included in the HDDL helpers section's version of this function)
    // For clarity, ensuring it's here:
    if (!primitive_tasks.empty()){
        domain_out << "; --- PDDL Primitive Actions (Not Transcribed Directly) ---" << std::endl;
        domain_out << "; The HDDL domain also includes the following PDDL primitive actions, which are referred to in method descriptions if they are subtasks:" << std::endl;
        std::vector<std::string> sorted_action_names_list; // Renamed to avoid conflict with set 'action_names'
        for(const auto& action_task : primitive_tasks){
            // Filter out internal actions like method preconditions if they are in primitive_tasks
            if (action_task.name.rfind(method_precondition_action_name, 0) != 0 &&
                action_task.name.rfind(immediate_method_precondition_action_name, 0) != 0) {
                sorted_action_names_list.push_back(action_task.name);
            }
        }
        std::sort(sorted_action_names_list.begin(), sorted_action_names_list.end());
        for(const auto& name : sorted_action_names_list) {
            domain_out << "; - " << name << std::endl;
        }
        domain_out << std::endl;
    }

    // Output for the problem file (typically minimal for this mode)
    problem_out << "; English Transcription for HDDL Problem File" << std::endl;
    problem_out << "; This mode primarily focuses on domain definitions (tasks and methods)." << std::endl;
    problem_out << "; Problem file content (initial state, goal tasks) is not transcribed in this version." << std::endl;
}
// --- End of Main Writer Function ---
