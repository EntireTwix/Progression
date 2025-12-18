#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
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
    std::cout << std::setprecision(1) << std::fixed;
    
    {
        std::string resp;
        double lower_weight_bound, upper_weight_bound, increment_distance;
    
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
                    while (std::getline(temp, line, ','))
                    {
                        line_d = std::stod(line);
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
                std::cout << "\nWhat is the smallest weight you have?\n";
                std::cin >> lower_weight_bound;
                std::cout << "\nWhat is the largest weight you have?\n";
                std::cin >> upper_weight_bound;
                std::cout <<"\nHow large is the increment between each weight in this range?\n";
                std::cin >> increment_distance;
                    
                for (double i = lower_weight_bound; i <= upper_weight_bound; i+=increment_distance)
                {
                    weight_and_rep.emplace(i, estimate_rm(i, max_weight));
                    rep_and_weight.emplace(estimate_rm(i, max_weight), i);
                }
            }
            else
            {
                std::cout << "\nPlease enter each weight you have seperated by a comma\n";
                std::cin >> resp;
                split(resp, ',', [&weight_and_rep, &rep_and_weight, max_weight](const std::string& str){ 
                    double temp = std::stod(str);
                    weight_and_rep.emplace(temp, estimate_rm(temp, max_weight)); 
                    rep_and_weight.emplace(estimate_rm(temp, max_weight), temp);
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
        
        if ((last.reps + last.rir >= rep_range_upp) || (last.reps + last.rir < rep_range_low) || (weight_and_rep.find(last.weight) == weight_and_rep.end()))
        {
            auto target_max = rep_and_weight.lower_bound(rep_range_low);
            // std::cout << std::abs(std::prev(target_max)->first - rep_range_low) << ' ' << std::abs(target_max->first - rep_range_low) << '\n';
            if ((std::abs(std::prev(target_max)->first - rep_range_low) < std::abs(target_max->first - rep_range_low)) 
            && (target_max != rep_and_weight.begin())) { --target_max; }
            target.weight = target_max->second;
            target.reps = target_max->first + 1 - target.rir;
        }
        else
        {
            target.weight = last.weight;
            target.reps = last.reps + last.rir + 1 - target.rir;
        }

        std::cout << "\n[Selecting " << target.weight << "lb as Working Weight]\n";
    }

    for (auto x : weight_and_rep)
    {
        std::cout << x.first << "lb " << x.second << "x " << (x.first / max_weight)*100 << "%\n";   
    }

    if (!weights_loaded && retrieve_response("\nWould you like to save the information of what weights you have available? (y/n)\n"))
    {
        std::ofstream temp("weights.txt");
        if (!temp.is_open()) { std::cout << "ERROR: Couldn't save weights to file\n"; }
        for (auto x : weight_and_rep)
        {
            std::cout << x.first << "lb " << x.second << "x " << (x.first / max_weight)*100 << "%\n";   
            temp << x.first << ',';
        }
    }

    std::cout << "\nEstimated 1RM based on last performance: " << max_weight << "lb\n";

    {
        int j = 1;
        double set_percentages[]{0.5, 0.6, 0.7625, 0.8 + (1/30.0), 0.958};
        for (double i : set_percentages)
        {
            if ((target.weight / max_weight) < i) { break; }
            auto warmup = weight_and_rep.lower_bound(i * max_weight);
            // std::cout << '\n' << i << ' ' << std::prev(warmup)->first << ' ' << warmup->first << ' ' << max_weight << '\n';
            if (std::abs(std::prev(warmup)->first - (i * max_weight)) < std::abs(warmup->first - (i * max_weight))) { --warmup; }
            
            // 0.958 is the point at which only 2.5x reps is possible therefore 35% of which would be 1
            if ((warmup->first / max_weight) > 0.958) { break; }
            std::cout << "\nWarmup " << j << ": " << warmup->first << "lb for " << unsigned(warmup->second * 0.4);
            ++j;
        }
    }

    std::cout << "\n\nWorking set: " << target.weight << "lb for " << target.reps << '\n';

    return 0;
}
