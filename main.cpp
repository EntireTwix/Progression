#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <unordered_set>
#include <map>
#include <cmath>
#include <iomanip>
#include <type_traits>

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
        else { std::cerr << "ERROR: invalid response, must be \"y\" or \"n\"\n"; }
    }while(1);
}

struct Performance
{
    double weight = 0;
    double reps = 0, rir = 0;
};

constexpr double brzycki_est(Performance x) { return x.weight / (1.0278 - (0.0278 * (x.reps + x.rir))); }
constexpr double epley_est(Performance x) { return x.weight * (1 + ((x.reps + x.rir) / 30.0)); }
constexpr double rev_brzycki_est(double weight, double max_weight) { return -(((weight / max_weight) - 1.0278) / 0.0278); }
constexpr double rev_epley_est(double weight, double max_weight) { return ((30 * max_weight) / weight) - 30; }

constexpr double estimate_rm(Performance x)
{
    double max_weight = 0;
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

constexpr double estimate_rm(double weight, double max_weight)
{
    double brzycki_threshold = max_weight * 0.8054;
    double epley_threshold = max_weight / (1.0 + (1.0 / 3.0));
    
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

//copy_fast type metafunction
template <typename T>
struct copy_fast : std::conditional<std::is_trivially_copyable<T>::value, T, const T &>{};
template <typename T>
using copy_fast_t = typename copy_fast<T>::type;

template <typename T, typename T2>
auto find_closet(const std::map<T, T2>& x, copy_fast_t<T> val)
{
    auto res = x.lower_bound(val);
    // std::cout << '\n' << res->first << ' ' << ' ' << res->second;
            
    if ((res != x.begin()) && 
    (std::abs(std::prev(res)->first - val) < std::abs(res->first - val)))
    { 
        --res;
        // std::cout << "->" << res->first << ' ' << res->second;
    }

    return res;
}

int find_precision(double x)
{
    int res = 0;
    for(; x != unsigned(x); x *= 10, ++res) { }
    return res;
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
                double increment;

                std::cout << "\nWhat is the smallest weight you have?\n";
                std::cin >> lower_weight_bound;
                std::cout << "\nWhat is the largest weight you have?\n";
                std::cin >> upper_weight_bound;
                std::cout <<"\nHow large is the increment between each weight in this range?\n";
                std::cin >> increment;
                
                precision_size += find_precision(increment);
                
                double temp_d;
                int temp_i;
                for (double i = lower_weight_bound; i <= upper_weight_bound; i+=increment)
                {
                    if ((temp_d = estimate_rm(i, max_weight)) < 0) { break; }
                    weight_and_rep.emplace(i, temp_d);
                    rep_and_weight.emplace(temp_d, i);
                    temp_i = find_precision(i);
                    if (temp_i > precision_size) { precision_size = temp_i; }
                }
            }
            else
            {
                std::cout << "\nPlease enter each weight you have seperated by a comma\n";
                std::cin >> resp;
                split(resp, ',', [&weight_and_rep, &rep_and_weight, &precision_size, max_weight](const std::string& str){ 
                    double temp_weight = std::stod(str), temp_est;
                    int temp_prec = find_precision(temp_weight);
                    
                    if ((temp_est = estimate_rm(temp_weight, max_weight)) >= 0)
                    {
                        weight_and_rep.emplace(temp_weight, temp_est); 
                        rep_and_weight.emplace(temp_est, temp_weight);
                        if (temp_prec > precision_size) { precision_size = temp_prec; }
                    }
                });
            }
        }
    }

    {
        double rep_range_low, rep_range_upp;
        std::cout << "\nHow many reps in reserve are you aiming for? (input 0 if unsure)\n";
        std::cin >> target.rir;
        std::cout << "\nWhat is the lower bound of your rep range? (input 8 if unsure)\n";
        std::cin >> rep_range_low;
        std::cout << "\nWhat is the upper bound of your rep range? (input 12 if unsure)\n";
        std::cin >> rep_range_upp;
        
        if ((last.reps + last.rir >= rep_range_upp) || (last.reps + last.rir + 1 < rep_range_low) || (weight_and_rep.find(last.weight) == weight_and_rep.end()))
        {
            rep_range_low = std::max(rep_range_low, target.rir);
            auto target_max = find_closet(rep_and_weight, rep_range_low);

            if (target_max->first <= target.rir)
            {
                if (target_max != rep_and_weight.end()) { ++target_max; }
                else { std::cerr << "ERROR: target RIR is larger than target reps in such a way that the program cannot adjust"; }
            }
            
            target.weight = target_max->second;
            target.reps = target_max->first + 1 - target.rir;
        }
        else
        {            
            target.weight = last.weight;
            target.reps = last.reps + last.rir + 1 - target.rir;
        }

        std::cout << std::setprecision(precision_size) << std::fixed
                  << "\n[Selecting " << target.weight << "lb as Working Weight]\n\n";
    }
    
    int largest_weight_length = std::to_string(unsigned(std::round(max_weight))).length() + precision_size + 1;
    int largest_rep_length = std::to_string(unsigned(weight_and_rep.begin()->second)).length();

    for (auto x : weight_and_rep)
    {
        std::cout << std::setw(largest_weight_length) << std::setprecision(precision_size) << std::right << x.first << "lb x "
                  << std::setw(largest_rep_length + 2) << std::setprecision(1) << std::left << x.second
                  << std::setw(8) << std::setprecision(2) << std::right << (x.first / max_weight)*100 << "%\n";   
    }

    if (!weights_loaded && retrieve_response("\nWould you like to save the information of what weights you have available? (y/n)\n"))
    {
        std::ofstream temp("weights.txt");
        if (!temp.is_open()) { std::cerr << "ERROR: Couldn't save weights to file\n"; }
        temp << precision_size << ',';
        for (auto x : weight_and_rep)
        {
            temp << x.first << ',';
        }
    }

    std::cout << "\nEstimated 1RM based on last performance: " << max_weight << "lb\n" << std::setprecision(precision_size);

    auto percentage_warmup = [max_weight, &weight_and_rep](double intended_percentage, unsigned intended_reps, std::vector<std::pair<double, double>>& sets){
        auto reduced_weight = intended_percentage * max_weight;
        auto reduced_rm = estimate_rm(reduced_weight, max_weight);
        auto closest_weight = find_closet(weight_and_rep, reduced_weight);
        sets.push_back({closest_weight->first, (intended_reps / reduced_rm) * closest_weight->second});
    };

    std::vector<std::pair<double, double>> warmup_sets;

    percentage_warmup(0.4, 5, warmup_sets);
    percentage_warmup(0.5, 5, warmup_sets);
    percentage_warmup(0.6, 3, warmup_sets);

    if ((target.weight / max_weight) >= 0.75)
    {
        percentage_warmup(0.75, 2, warmup_sets);
    
        if ((target.weight / max_weight) >= 0.89)
        {
            percentage_warmup(0.89, 1, warmup_sets);
        }
    }

    size_t j = 1;
    for (auto set : warmup_sets)
    {
        if (set.second < 0.5) { break; }
        // ok to round up for warm up because very submaximal load
        std::cout << "\nWarmup " << j++ << "    : " << std::setw(largest_weight_length) << set.first << "lb for " << unsigned(std::round(set.second));
    }

    // not ok to round up for rep because 0.9 reps for a weight where you fail sticking point is not a 1 RM for example
    std::cout << "\n\nWorking set : " << std::setw(largest_weight_length) << target.weight << "lb for " << unsigned(target.reps) << '\n';

    return 0;
}
