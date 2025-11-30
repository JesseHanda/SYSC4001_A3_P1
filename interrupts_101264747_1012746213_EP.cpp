/**
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 * @brief template main.cpp file for Assignment 3 Part 1 of SYSC4001
 * 
 */

#include "interrupts_101264747_1012746213.hpp"

// Helper: pick highest-priority READY process (lower priority value = higher)
static void dispatch_highest_priority(
    PCB &running,
    std::vector<PCB> &job_list,
    std::vector<PCB> &ready_queue,
    unsigned int current_time
) {
    // Find element with minimum priority among items available now
    auto best_it = ready_queue.end();
    unsigned int best_prio = UINT_MAX;
    for (auto it = ready_queue.begin(); it != ready_queue.end(); ++it) {
        if (it->available_time <= current_time) {
            if (best_it == ready_queue.end() || it->priority < best_prio) {
                best_it = it;
                best_prio = it->priority;
            }
        }
    }

    if (best_it == ready_queue.end()) return; // nothing available now

    PCB next = *best_it;
    ready_queue.erase(best_it);

    running = next;

    if (running.start_time == -1) {
        running.start_time = static_cast<int>(current_time);
    }

    running.state = RUNNING;
    sync_queue(job_list, running);
}

std::tuple<std::string> run_simulation(std::vector<PCB> list_processes) {

    std::vector<PCB> ready_queue;
    std::vector<PCB> wait_queue;
    std::vector<PCB> job_list;

    unsigned int current_time = 0;
    PCB running;
    idle_CPU(running);

    std::string execution_status = print_exec_header();

    while (!all_process_terminated(job_list) || job_list.empty()) {

        // --------------------------------------------------
        // (1) ARRIVALS
        // --------------------------------------------------
        for (auto &process : list_processes) {
            if (process.arrival_time == current_time) {

                bool loaded = assign_memory(process);
                if (!loaded) {
                    // For simplicity, skip if no memory; could retry later.
                    continue;
                }

                // External priority: by default, we used PID as priority
                // (already set in add_process), but you can adjust here
                // if you want a different rule.

                process.state = READY;
                process.time_since_last_io = 0;
                process.io_remaining       = 0;

                ready_queue.push_back(process);
                job_list.push_back(process);

                execution_status +=
                    print_exec_status(current_time, process.PID, NEW, READY);
            }
        }

        // --------------------------------------------------
        // (2) MANAGE WAIT QUEUE (I/O completion)
        // --------------------------------------------------
        for (auto it = wait_queue.begin(); it != wait_queue.end(); ) {

            it->io_remaining--;

            if (it->io_remaining == 0) {
                // WAITING -> READY
                states old_state = WAITING;
                it->state = READY;
                it->time_since_last_io = 0;
                it->available_time = current_time + 1;

                sync_queue(job_list, *it);
                ready_queue.push_back(*it);

                // Report the READY transition at the end of this millisecond
                execution_status +=
                    print_exec_status(current_time + 1,
                                      it->PID,
                                      old_state,
                                      READY);

                it = wait_queue.erase(it);
            } else {
                ++it;
            }
        }

        // --------------------------------------------------
        // (3) DISPATCH IF CPU IDLE (no preemption)
        // --------------------------------------------------
        if (running.PID == -1 && !ready_queue.empty()) {

            dispatch_highest_priority(running, job_list, ready_queue, current_time);

            if (running.PID != -1) {
                execution_status +=
                    print_exec_status(current_time, running.PID, READY, RUNNING);
            }
        }

        // --------------------------------------------------
        // (4) EXECUTE 1 ms OF CURRENT PROCESS
        // --------------------------------------------------
        if (running.PID != -1) {

            running.remaining_time--;

            if (running.io_freq > 0) {
                running.time_since_last_io++;
            }

            sync_queue(job_list, running);

            // (a) Finished?
            if (running.remaining_time == 0) {

                states old_state = RUNNING;
                terminate_process(running, job_list);

                // Report the termination at the end of this millisecond
                execution_status +=
                    print_exec_status(current_time + 1,
                                      running.PID,
                                      old_state,
                                      TERMINATED);

                running.PID = -1;
            }
            // (b) Needs I/O? (RUNNING -> WAITING)
            else if (running.io_freq > 0 &&
                     running.time_since_last_io >= running.io_freq) {

                states old_state = RUNNING;
                running.state = WAITING;
                running.io_remaining = running.io_duration;
                running.time_since_last_io = 0;

                sync_queue(job_list, running);
                wait_queue.push_back(running);

                // Report the WAITING transition at the end of this millisecond
                execution_status +=
                    print_exec_status(current_time + 1,
                                      running.PID,
                                      old_state,
                                      WAITING);

                running.PID = -1;
            }
            // NOTE: no quantum / preemption here for EP
        }

        // --------------------------------------------------
        // (5) Safety break if nothing exists and nothing will arrive
        // --------------------------------------------------
        if (job_list.empty() && ready_queue.empty() && wait_queue.empty() && running.PID == -1) {
            bool any_future_arrivals = false;
            for (auto &p : list_processes) {
                if (p.arrival_time > current_time) {
                    any_future_arrivals = true;
                    break;
                }
            }
            if (!any_future_arrivals) {
                break;
            }
        }

        current_time++;
    }

    execution_status += print_exec_footer();
    return std::make_tuple(execution_status);
}

int main(int argc, char **argv) {

    if (argc != 2) {
        std::cout << "ERROR!\nExpected 1 argument, received "
                  << argc - 1 << std::endl;
        std::cout << "To run the program, do: ./interrupts_EP <your_input_file.txt>"
                  << std::endl;
        return -1;
    }

    auto file_name = argv[1];
    std::ifstream input_file(file_name);

    if (!input_file.is_open()) {
        std::cerr << "Error: Unable to open file: " << file_name << std::endl;
        return -1;
    }

    std::string line;
    std::vector<PCB> list_process;
    while (std::getline(input_file, line)) {
        if (line.empty()) continue;
        auto input_tokens = split_delim(line, ", ");
        auto new_process = add_process(input_tokens);
        list_process.push_back(new_process);
    }
    input_file.close();

    auto [exec] = run_simulation(list_process);

    // derive an output filename from the input file name so each test
    // writes its own execution file (e.g., test1.txt -> execution1.txt)
    std::string inname = file_name;
    std::string base = inname;
    // strip any path
    auto pos = base.find_last_of("/\\");
    if (pos != std::string::npos) base = base.substr(pos + 1);
    // strip extension
    pos = base.find_last_of('.');
    if (pos != std::string::npos) base = base.substr(0, pos);

    std::string outname;
    if (base.rfind("test", 0) == 0 && base.size() > 4) {
        // testN -> executionN
        outname = std::string("execution") + base.substr(4) + ".txt";
    } else {
        outname = std::string("execution_") + base + ".txt";
    }

    write_output(exec, outname.c_str());

    return 0;
}