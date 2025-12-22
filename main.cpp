#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <unordered_set>
#include <map>
#include <cmath>
#include <iomanip>

template <typename T>
inline std::vector<std::string> split(const std::string &inp, char delim, T &&func)
{
    std::stringstream ss(inp);
    std::vector<std::string> res;
    std::string line;
    while (std::getline(ss, line, delim))
    {
        func(line);
        res.push_back(line);
    }
    return res;
}

bool retrieve_response(const std::string& question)
{
    std::string str;
    do
    {
        std::cout << question;
        std::cin >> str; 
        if (str == "y") { return true; }
        else if(str == "n") { return false; }
        else { std::cout << "ERROR: invalid response, must be \"y\" or \"n\"\n"; }
    }while(1);
}

struct Performance
{
    double weight = 0;
    unsigned reps = 0, rir = 0;
};

constexpr double brzycki_est(Performance x) { return x.weight / (1.0278 - (0.0278 * (x.reps + x.rir))); }
constexpr double epley_est(Performance x) { return x.weight * (1 + ((x.reps + x.rir) / 30.0)); }
constexpr double rev_brzycki_est(double weight, double max_weight) { return -(((weight / max_weight) - 1.0278) / 0.0278); }
constexpr double rev_epley_est(double weight, double max_weight) { return ((30 * max_weight) / weight) - 30; }

double estimate_rm(Performance x)
{
    double max_weight;
    unsigned rep_adj = x.reps + x.rir;
    
    if (rep_adj <= 8)
    {
        max_weight = brzycki_est(x);
    }
    else if (rep_adj >= 10)
    {
        max_weight = epley_est(x);
    }
    else
    {
        max_weight = (brzycki_est(x) + epley_est(x)) / 2;
    }
    
    return max_weight;
}

double estimate_rm(double weight, double max_weight)
{
    double brzycki_threshold = max_weight * 0.8054;
    double epley_threshold = max_weight / (1 + (1/3.0));
    
    if (weight >= brzycki_threshold)
    {
        return rev_brzycki_est(weight, max_weight);
    }
    else if (weight <= epley_threshold)
    {
        return rev_epley_est(weight, max_weight);
    }
    else
    {
        double epley_mult = (brzycki_threshold - weight) / (brzycki_threshold - epley_threshold);
        
        return (epley_mult * rev_epley_est(weight, max_weight)) + ((1 - epley_mult) * rev_brzycki_est(weight, max_weight));
    }
}

int main()
{
    Performance last, target;
    double max_weight;
    std::map<double, double> weight_and_rep, rep_and_weight;
    bool weights_loaded = false;
    std::cout << std::fixed;
    int precision_size = 0;
    
    {
        std::string resp;
        double lower_weight_bound, upper_weight_bound;
    
        std::cout << "How much weight did you lift last performance?\n";
        std::cin >> last.weight;
        std::cout << "\nHow many reps of this weight did you perform?\n";
        std::cin >> last.reps;
        std::cout << "\nHow many reps in reserve do you estimate you had? (input 0 if unsure)\n";
        std::cin >> last.rir;
        max_weight = estimate_rm(last);

        {
            std::ifstream temp("weights.txt");
            double line_d;
            std::string line;
            if (temp.is_open())
            {
                if (retrieve_response("\nWould you like to load your stored weight values? (y/n)\n"))                   
                {
                    
                    std::getline(temp, line, ',');
                    int line_i = std::stoi(line);
                    precision_size = line_i;
                    
                    double temp_est;
                    while (std::getline(temp, line, ','))
                    {
                        line_d = std::stod(line);
                        temp_est = estimate_rm(line_d, max_weight);
                        if (temp_est < 0) { break; }
                        weight_and_rep.emplace(line_d, estimate_rm(line_d, max_weight));
                        rep_and_weight.emplace(estimate_rm(line_d, max_weight), line_d);
                    }
                    temp.close();
                    weights_loaded = true;
                }
            }
        }
        
        if (!weights_loaded)
        {
            if (retrieve_response("\nDo you have a continous range of weights available for this excercise? (y/n)\n"))
            {
                std::string increment_distance;

                std::cout << "\nWhat is the smallest weight you have?\n";
                std::cin >> lower_weight_bound;
                std::cout << "\nWhat is the largest weight you have?\n";
                std::cin >> upper_weight_bound;
                std::cout <<"\nHow large is the increment between each weight in this range?\n";
                std::cin >> increment_distance;
                
                auto distance = increment_distance.find('.');
                precision_size += (distance != std::string::npos) * (increment_distance.length() - distance - 1);
                double temp, inc = std::stod(increment_distance);
                
                for (double i = lower_weight_bound; i <= upper_weight_bound; i+=inc)
                {
                    if ((temp = estimate_rm(i, max_weight)) < 0) { break; }
                    weight_and_rep.emplace(i, temp);
                    rep_and_weight.emplace(temp, i);
                }
            }
            else
            {
                std::cout << "\nPlease enter each weight you have seperated by a comma\n";
                std::cin >> resp;
                split(resp, ',', [&weight_and_rep, &rep_and_weight, max_weight](const std::string& str){ 
                    double temp_weight = std::stod(str), temp_est;
                    if ((temp_est = estimate_rm(temp_weight, max_weight)) >= 0)
                    {
                        weight_and_rep.emplace(temp_weight, temp_est); 
                        rep_and_weight.emplace(temp_est, temp_weight);
                    }
                });
            }
        }
    }

    {
        unsigned rep_range_low, rep_range_upp;
        std::cout << "\nHow many reps in reserve are you aiming for? (input 0 if unsure)\n";
        std::cin >> target.rir;
        std::cout << "\nWhat is the lower bound of your rep range? (input 8 if unsure)\n";
        std::cin >> rep_range_low;
        std::cout << "\nWhat is the upper bound of your rep range? (input 12 if unsure)\n";
        std::cin >> rep_range_upp;
        
        if ((last.reps + last.rir >= rep_range_upp) || (last.reps + last.rir + 1 < rep_range_low) || (weight_and_rep.find(last.weight) == weight_and_rep.end()))
        {
            std::cout << "Last performance's weight is not suitable, finding another\n";

            auto target_max = rep_and_weight.lower_bound(rep_range_low);
            // std::cout << '\n' << target_max->first << ' ' << ' ' << target_max->second;
            
            if ((target_max != rep_and_weight.begin()) && 
            (std::abs(std::prev(target_max)->first - rep_range_low) < std::abs(target_max->first - rep_range_low)))
            { 
                --target_max;
                // std::cout << "->" << target_max->first << ' ' << target_max->second;
            }
            
            target.weight = target_max->second;
            target.reps = target_max->first + 1 - target.rir;

        }
        else
        {
            std::cout << "\n[Last performance's weight is available and within rep range]\n";
            
            target.weight = last.weight;
            target.reps = last.reps + last.rir + 1 - target.rir;
        }

        std::cout << std::setprecision(precision_size) << std::fixed
                  << "[Selecting " << target.weight << "lb as Working Weight]\n\n";
    }
    
    int largest_weight_length = std::to_string(unsigned(std::round(max_weight))).length() + precision_size + 1;
    int largest_rep_length = std::to_string(unsigned(weight_and_rep.begin()->second)).length();

    for (auto x : weight_and_rep)
    {
        std::cout << "| "
                  << std::setw(largest_weight_length) << std::setprecision(precision_size) << x.first << "lb | "
                  << std::setw(largest_rep_length + 2) << std::setprecision(1) << x.second << "x | "
                  << std::setw(6) << std::setprecision(2) << (x.first / max_weight)*100 << "% |\n";   
    }

    if (!weights_loaded && retrieve_response("\nWould you like to save the information of what weights you have available? (y/n)\n"))
    {
        std::ofstream temp("weights.txt");
        if (!temp.is_open()) { std::cout << "ERROR: Couldn't save weights to file\n"; }
        temp << precision_size << ',';
        for (auto x : weight_and_rep)
        {
            temp << x.first << ',';
        }
    }

    std::cout << "\nEstimated 1RM based on last performance: " << max_weight << "lb\n" << std::setprecision(precision_size);

    {
        int j = 1;
        // 12 8 4 2 1
        double set_percentages[]{0.5, 0.6, 0.75, 0.89, 0.958};
        std::unordered_set<double> warmups;
        for (double i : set_percentages)
        {
            if ((target.weight / max_weight) < i) { break; }
            auto warmup = weight_and_rep.lower_bound(i * max_weight);
            // std::cout << '\n' << i << ' ' << std::prev(warmup)->first << ' ' << warmup->first << ' ' << max_weight << '\n';
            if (warmup != weight_and_rep.begin() && std::abs(std::prev(warmup)->first - (i * max_weight)) < std::abs(warmup->first - (i * max_weight))) { --warmup; }
            
            // 0.958 is the point at which only 2.5x reps is possible therefore 40% of which would be 1
            if (((warmup->first / max_weight) > 0.958) || !warmups.emplace(warmup->first).second) { continue; }
            std::cout << "\nWarmup " << j << "    : " << std::setw(largest_weight_length) << warmup->first << "lb for " << unsigned(warmup->second * 0.4);
            ++j;
        }
    }

    std::cout << "\n\nWorking set : " << std::setw(largest_weight_length) << target.weight << "lb for " << target.reps << '\n';

    return 0;
}
