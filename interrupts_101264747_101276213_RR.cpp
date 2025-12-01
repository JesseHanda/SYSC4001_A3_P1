/**
 * @file interrupts_101264747_101276213_RR.cpp
 * @author Jesse Handa
 * @brief template main.cpp file for Assignment 3 Part 1 of SYSC4001
 *
 */

#include "interrupts_101264747_101276213.hpp"

// Helper: find highest-priority ready process
static void dispatch_highest_priority(
    PCB &running,
    std::vector<PCB> &job_list,
    std::vector<PCB> &ready_queue,
    unsigned int current_time)
{
    auto iterator1 = ready_queue.end();
    unsigned int priority1 = UINT_MAX;
    for (auto iter = ready_queue.begin(); iter != ready_queue.end(); ++iter)
    {
        if (iter->available_time <= current_time)
        {
            if (iterator1 == ready_queue.end() || iter->priority < priority1)
            {
                iterator1 = iter;
                priority1 = iter->priority;
            }
        }
    }

    if (iterator1 == ready_queue.end())
        return;

    PCB next = *iterator1;
    ready_queue.erase(iterator1);

    running = next;

    if (running.start_time == -1)
    {
        running.start_time = static_cast<int>(current_time);
    }

    running.state = RUNNING;
    sync_queue(job_list, running);
}

std::tuple<std::string> run_simulation(std::vector<PCB> list_processes)
{

    std::vector<PCB> ready_queue;
    std::vector<PCB> wait_queue;
    std::vector<PCB> job_list;

    unsigned int current_time = 0;
    PCB running;
    idle_CPU(running);

    std::string execution_status = print_exec_header();

    while (!all_process_terminated(job_list) || job_list.empty())
    {

        // arrive
        for (auto &process : list_processes)
        {
            if (process.arrival_time == current_time)
            {
                bool loaded = assign_memory(process);
                if (!loaded)
                {
                    continue;
                }
                // PID is priority
                process.state = READY;
                process.time_since_last_io = 0;
                process.io_remaining = 0;
                ready_queue.push_back(process);
                job_list.push_back(process);
                execution_status +=
                    print_exec_status(current_time, process.PID, NEW, READY);
            }
        }

        // IO completion
        for (auto iter = wait_queue.begin(); iter != wait_queue.end();)
        {
            iter->io_remaining--;
            if (iter->io_remaining == 0)
            {
                states old_state = WAITING;
                iter->state = READY;
                iter->time_since_last_io = 0;
                iter->available_time = current_time + 1;
                sync_queue(job_list, *iter);
                ready_queue.push_back(*iter);
                // print ready
                execution_status +=
                    print_exec_status(current_time + 1,
                                      iter->PID,
                                      old_state,
                                      READY);
                iter = wait_queue.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
        // dispatch
        if (running.PID == -1 && !ready_queue.empty())
        {
            dispatch_highest_priority(running, job_list, ready_queue, current_time);
            if (running.PID != -1)
            {
                execution_status +=
                    print_exec_status(current_time, running.PID, READY, RUNNING);
            }
        }
        // execute
        if (running.PID != -1)
        {
            running.remaining_time--;
            if (running.io_freq > 0)
            {
                running.time_since_last_io++;
            }
            sync_queue(job_list, running);
            if (running.remaining_time == 0)
            { // if finished
                states old_state = RUNNING;
                terminate_process(running, job_list);
                // print termination
                execution_status +=
                    print_exec_status(current_time + 1,
                                      running.PID,
                                      old_state,
                                      TERMINATED);

                running.PID = -1;
            }
            // if needs IO: running->waiting
            else if (running.io_freq > 0 &&
                     running.time_since_last_io >= running.io_freq)
            {
                states old_state = RUNNING;
                running.state = WAITING;
                running.io_remaining = running.io_duration;
                running.time_since_last_io = 0;
                sync_queue(job_list, running);
                wait_queue.push_back(running);
                // Print waiting
                execution_status +=
                    print_exec_status(current_time + 1,
                                      running.PID,
                                      old_state,
                                      WAITING);

                running.PID = -1;
            }
        }

        // if nothing there
        if (job_list.empty() && ready_queue.empty() && wait_queue.empty() && running.PID == -1)
        {
            bool any_future_arrivals = false;
            for (auto &p : list_processes)
            {
                if (p.arrival_time > current_time)
                {
                    any_future_arrivals = true;
                    break;
                }
            }
            if (!any_future_arrivals)
            {
                break;
            }
        }
        current_time++;
    }
    execution_status += print_exec_footer();
    return std::make_tuple(execution_status);
}
int main(int argc, char **argv)
{

    if (argc != 2)
    {
        std::cout << "ERROR!\nExpected 1 argument, received "
                  << argc - 1 << std::endl;
        std::cout << "To run the program, do: ./interrupts_EP <your_input_file.txt>"
                  << std::endl;
        return -1;
    }

    auto file_name = argv[1];
    std::ifstream input_file(file_name);

    if (!input_file.is_open())
    {
        std::cerr << "Error: Unable to open file: " << file_name << std::endl;
        return -1;
    }

    std::string line;
    std::vector<PCB> list_process;
    while (std::getline(input_file, line))
    {
        if (line.empty())
            continue;
        auto input_tokens = split_delim(line, ", ");
        auto new_process = add_process(input_tokens);
        list_process.push_back(new_process);
    }
    input_file.close();

    auto [exec] = run_simulation(list_process);

    std::string inname = file_name;
    std::string base = inname;
    auto pos = base.find_last_of("/\\");
    if (pos != std::string::npos)
        base = base.substr(pos + 1);
    pos = base.find_last_of('.');
    if (pos != std::string::npos)
        base = base.substr(0, pos);

    std::string outname;
    if (base.rfind("test", 0) == 0 && base.size() > 4)
    {
        outname = std::string("execution") + base.substr(4) + ".txt";
    }
    else
    {
        outname = std::string("execution_") + base + ".txt";
    }

    write_output(exec, outname.c_str());

    return 0;
}
