/**
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 * @brief template main.cpp file for Assignment 3 Part 1 of SYSC4001
 * 
 */

/**
 * @file interrupts_student1_student2_RR.cpp
 * @brief Round Robin (RR) scheduler implementation for Assignment 3 Part 1
 */

#include "interrupts_101264747_101276213.hpp"

// Helper: find highest-priority ready process

static std::vector<PCB>::iterator find_best_ready(std::vector<PCB> &ready_queue, unsigned int current_time) {
    if (ready_queue.empty()) return ready_queue.end();

    auto iterator1 = ready_queue.end();
    unsigned int best_prio = UINT_MAX;

    for (auto iter = ready_queue.begin(); iter != ready_queue.end(); ++iter) {
        if (iter->available_time <= current_time) {
            if (iterator1 == ready_queue.end() || iter->priority < best_prio) {
                best_prio = iter->priority;
                iterator1 = iter;
            }
        }
    }
    return iterator1;
}

// Dispatch: ready -> running
static bool dispatch_best(
    PCB &running,
    std::vector<PCB> &job_list,
    std::vector<PCB> &ready_queue,
    unsigned int current_time,
    std::string &execution_status
) {
    auto iterator1 = find_best_ready(ready_queue, current_time);
    if (iterator1 == ready_queue.end()) return false;

    PCB next = *iterator1;
    ready_queue.erase(iterator1);

    running = next;

    if (running.start_time == -1) {
        running.start_time = static_cast<int>(current_time);
    }

    running.state = RUNNING;
    sync_queue(job_list, running);

    execution_status +=
        print_exec_status(current_time, running.PID, READY, RUNNING);

    return true;
}

std::tuple<std::string> run_simulation(std::vector<PCB> list_processes) {

    std::vector<PCB> ready_queue;
    std::vector<PCB> wait_queue;
    std::vector<PCB> job_list;

    unsigned int current_time = 0;
    PCB running;
    idle_CPU(running);

    std::string execution_status = print_exec_header();

    const unsigned int QUANT = 100;
    unsigned int time_slice = 0;

    while (!all_process_term(job_list) || job_list.empty()) {

        // arrive: new -> ready
        for (auto &process : list_processes) {
            if (process.arrival_time == current_time) {

                bool loaded = assign_memory(process);
                if (!loaded) {
                    // if no mem: skip
                    continue;
                }
                process.state = READY;
                process.time_since_last_io = 0;
                process.io_remaining       = 0;
                ready_queue.push_back(process);
                job_list.push_back(process);
                execution_status +=
                    print_exec_status(current_time, process.PID, NEW, READY);
            }
        }

        // IO completion: waiting -> ready
        for (auto iter = wait_queue.begin(); iter != wait_queue.end(); ) {
            iter->io_remaining--;
            if (iter->io_remaining == 0) {
                states old_state = WAITING;
                iter->state = READY;
                iter->time_since_last_io = 0;
                iter->available_time = current_time + 1;
                sync_queue(job_list, *iter);
                ready_queue.push_back(*iter);
                // print
                execution_status +=
                    print_exec_status(current_time + 1,
                                      iter->PID,
                                      old_state,
                                      READY);
                iter = wait_queue.erase(iter);
            } else {
                ++iter;
            }
        }
        // (3) preemption and dispatch
        if (!ready_queue.empty()) {
            auto iterator1 = find_best_ready(ready_queue, current_time);
            if (iterator1 == ready_queue.end()) {
                // if no process: idle
            } else {
                if (running.PID == -1) {
                    // dispatch
                    dispatch_best(running, job_list, ready_queue, current_time, execution_status);
                    if (running.PID != -1) {
                        time_slice = 0;
                    }
                } else {
                    if (iterator1->priority < running.priority) {
                        states old_state = RUNNING;
                        running.state = READY;
                        sync_queue(job_list, running);
                        ready_queue.push_back(running);
                        execution_status +=
                            print_exec_status(current_time,
                                              running.PID,
                                              old_state,
                                              READY);
                        running.PID = -1;
                        time_slice  = 0;
                        // disparch
                        dispatch_best(running, job_list, ready_queue, current_time, execution_status);
                    }
                }
            }
        } else if (running.PID == -1) {
        }

        // (4) execute current process
        if (running.PID != -1) {
            running.remaining_time--;
            if (running.io_freq > 0) {
                running.time_since_last_io++;
            }
            sync_queue(job_list, running);
            bool currRunning = true;
            if (running.remaining_time == 0) { // if done
                states old_state = RUNNING;
                terminate_process(running, job_list);
                // Print termination
                execution_status +=
                    print_exec_status(current_time + 1,
                                      running.PID,
                                      old_state,
                                      TERMINATED);

                running.PID = -1;
                time_slice  = 0;
                currRunning = false;
            }
            else if (running.io_freq > 0 && running.time_since_last_io >= running.io_freq) { // if needs IO: running->waiting

                states old_state = RUNNING;
                running.state = WAITING;
                running.io_remaining = running.io_duration;
                running.time_since_last_io = 0;
                sync_queue(job_list, running);
                wait_queue.push_back(running);

                // print waiting
                execution_status +=
                    print_exec_status(current_time + 1,
                                      running.PID,
                                      old_state,
                                      WAITING);
                running.PID = -1;
                time_slice  = 0;
                currRunning = false;
            }
            // if quant is reached: running->ready
            if (currRunning) {
                time_slice++;
                if (time_slice == QUANT) {
                    states old_state = RUNNING;
                    running.state = READY;
                    sync_queue(job_list, running);
                    ready_queue.push_back(running);
                    execution_status +=
                        print_exec_status(current_time,
                                          running.PID,
                                          old_state,
                                          READY);
                    running.PID = -1;
                    time_slice  = 0;
                }
            }
        }
        // if idle and nothing there (just in case)
        if (job_list.empty() && ready_queue.empty() && wait_queue.empty() && running.PID == -1) {
            bool newArrivals = false;
            for (auto &p : list_processes) {
                if (p.arrival_time > current_time) {
                    newArrivals = true;
                    break;
                }
            }
            if (!newArrivals) {
                break;
            }
        }
        current_time++;
    }
    execution_status += print_exec_footer();
    return std::make_tuple(execution_status);
}

int main(int argc, char** argv) {

    //Get the input file from the user
    if(argc != 2) {
        std::cout << "ERROR!\nExpected 1 argument, received " << argc - 1 << std::endl;
        std::cout << "To run the program, do: ./interrutps <your_input_file.txt>" << std::endl;
        return -1;
    }

    //Open the input file
    auto file_name = argv[1];
    std::ifstream input_file;
    input_file.open(file_name);

    //Ensure that the file actually opens
    if (!input_file.is_open()) {
        std::cerr << "Error: Unable to open file: " << file_name << std::endl;
        return -1;
    }

    //Parse the entire input file and populate a vector of PCBs.
    //To do so, the add_process() helper function is used (see include file).
    std::string line;
    std::vector<PCB> list_process;
    while(std::getline(input_file, line)) {
        auto input_tokens = split_delim(line, ", ");
        auto new_process = add_process(input_tokens);
        list_process.push_back(new_process);
    }
    input_file.close();

    //With the list of processes, run the simulation
    auto [exec] = run_simulation(list_process);

    write_output(exec, "execution.txt");

    return 0;
}