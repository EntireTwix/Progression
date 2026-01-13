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
    long double weight = 0;
    long double reps = 0, rir = 0;
};

constexpr long double brzycki_est(Performance x) { return x.weight / (1.0278 - (0.0278 * (x.reps + x.rir))); }
constexpr long double epley_est(Performance x) { return x.weight * (1 + ((x.reps + x.rir) / 30.0)); }
constexpr long double rev_brzycki_est(long double weight, long double max_weight) { return -(((weight / max_weight) - 1.0278) / 0.0278); }
constexpr long double rev_epley_est(long double weight, long double max_weight) { return ((30 * max_weight) / weight) - 30; }

constexpr long double estimate_rm(Performance x)
{
    long double max_weight = 0;
    long double rep_adj = x.reps + x.rir;
    
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

constexpr long double estimate_rm(long double weight, long double max_weight)
{
    long double brzycki_threshold = max_weight * 0.8054;
    long double epley_threshold = max_weight / (1.0 + (1.0 / 3.0));
    
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
        long double epley_mult = (brzycki_threshold - weight) / (brzycki_threshold - epley_threshold);
        
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

size_t find_precision(const std::string& x) 
{ 
    auto location = x.find('.');
    if (location != std::string::npos)
    {
        return x.length() - location - 1; 
    }
    else
    {
        return 0;
    }
}

int main()
{
    Performance last, target;
    long double max_weight;
    std::map<long double, long double> weight_and_rep, rep_and_weight;
    bool weights_loaded = false;
    std::cout << std::fixed;
    int precision_size = 0;
    
    {
        std::string resp;
    
        std::cout << "How much weight did you lift last performance?\n";
        std::cin >> last.weight;
        std::cout << "\nHow many reps of this weight did you perform?\n";
        std::cin >> last.reps;
        std::cout << "\nHow many reps in reserve do you estimate you had? (input 0 if unsure)\n";
        std::cin >> last.rir;
        max_weight = estimate_rm(last);

        {
            std::ifstream temp("weights.txt");
            long double line_d;
            std::string line;
            if (temp.is_open())
            {
                std::cout << "[Found Weights File]\n";
                if (retrieve_response("\nWould you like to load your stored weight values? (y/n)\n"))                   
                {
                    
                    std::getline(temp, line, ',');
                    int line_i = std::stoi(line);
                    precision_size = line_i;
                    
                    long double temp_est;
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

                    std::cout << "[Weights Loaded]\n";
                }
            }
        }
        
        if (!weights_loaded)
        {
            if (retrieve_response("\nDo you have a continous range of weights available for this excercise? (y/n)\n"))
            {
                std::string lower_weight_bound_str, upper_weight_bound_str, increment_str;
                long double lower_weight_bound, upper_weight_bound, increment;

                std::cout << "\nWhat is the smallest weight you have?\n";
                std::cin >> lower_weight_bound_str;
                std::cout << "\nWhat is the largest weight you have?\n";
                std::cin >> upper_weight_bound_str;
                std::cout <<"\nHow large is the increment between each weight in this range?\n";
                std::cin >> increment_str;

                lower_weight_bound = std::stod(lower_weight_bound_str);
                upper_weight_bound = std::stod(upper_weight_bound_str);
                increment = std::stod(increment_str);
                
                precision_size += std::max(find_precision(increment_str), std::max(find_precision(lower_weight_bound_str), find_precision(upper_weight_bound_str)));
                if (precision_size) { std::cout << "\n[Updating Display Precision to " << precision_size << "]"; }

                std::cout << "\n[Generating Weights Range]\n";
                long double temp_est, temp_weight;
                for (size_t i = 0; i < ((upper_weight_bound - lower_weight_bound) / increment) + 1; ++i)
                {
                    temp_weight = lower_weight_bound + (i * increment);
                    if ((temp_est = estimate_rm(temp_weight, max_weight)) < 0) { break; }
                    weight_and_rep.emplace(temp_weight, temp_est);
                    rep_and_weight.emplace(temp_est, temp_weight);
                }
            }
            else
            {
                std::cout << "\nPlease enter each weight you have seperated by a comma\n";
                std::cin >> resp;
                split(resp, ',', [&weight_and_rep, &rep_and_weight, &precision_size, max_weight](const std::string& str){ 
                    int temp_prec = find_precision(str);
                    long double temp_weight = std::stod(str), temp_est;
                    
                    if ((temp_est = estimate_rm(temp_weight, max_weight)) >= 0)
                    {
                        std::cout << "[Loaded " + str << "lb with Rep Estimate of " << temp_est << "]\n";
                        weight_and_rep.emplace(temp_weight, temp_est); 
                        rep_and_weight.emplace(temp_est, temp_weight);
                        if (temp_prec > precision_size) 
                        {
                            precision_size = temp_prec; 
                            std::cout << "[Updating Display Precision to " << temp_prec << " to Represent " << str << "]\n"; 
                        }
                    }
                });
            }
        }
        std::cout << '[' << weight_and_rep.size() << " Weights Available]\n";
    }

    {
        long double rep_range_low, rep_range_upp;
        std::cout << "\nHow many reps in reserve are you aiming for? (input 0 if unsure)\n";
        std::cin >> target.rir;
        std::cout << "\nWhat is the lower bound of your rep range? (input 8 if unsure)\n";
        std::cin >> rep_range_low;
        std::cout << "\nWhat is the upper bound of your rep range? (input 12 if unsure)\n";
        std::cin >> rep_range_upp;
        
        if ((last.reps + last.rir >= rep_range_upp) || (last.reps + last.rir + 1 < rep_range_low) || (weight_and_rep.find(last.weight) == weight_and_rep.end()))
        {
            std::cout << "[Finding Appropriate Weight for Desired Rep Range]\n";
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
            std::cout << "[Using Last Performance's Weight]\n";
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

    
    auto percentage_warmup = [max_weight, &weight_and_rep](long double intended_percentage, long double intended_reps, std::vector<std::pair<long double, long double>>& sets) {

        auto reduced_weight = intended_percentage * max_weight;
        auto reduced_rm = estimate_rm(reduced_weight, max_weight);
        auto closest_weight = find_closet(weight_and_rep, reduced_weight);
        
        std::cout << "[Last Weight's Reps " << sets.back().second << " vs This Weight's Reps " << ((intended_reps / reduced_rm) * closest_weight->second) << "]\n";
        if ((sets.back().first == closest_weight->first) && (unsigned(std::round(sets.back().second)) >= unsigned(std::round((intended_reps / reduced_rm) * closest_weight->second))))
        {
            std::cout << "[Erasing " << closest_weight->first << "lb from Available Weights]\n";
            
            weight_and_rep.erase(closest_weight);
            reduced_weight = intended_percentage * max_weight;
            reduced_rm = estimate_rm(reduced_weight, max_weight);
            closest_weight = find_closet(weight_and_rep, reduced_weight);
        }

        if (closest_weight != weight_and_rep.end())
        {
            sets.emplace_back(closest_weight->first, (intended_reps / reduced_rm) * closest_weight->second);
            std::cout << "[Looking for " << intended_percentage * 100 << "% of 1RM or " << reduced_weight 
                  << "lb]\n[Estimated Rep Max for " << reduced_weight << "lb is " << reduced_rm
                  << "]\n[Closest Weight Found: " << closest_weight->first << "lb for " << closest_weight->second << "]\n"
                  << "[" << intended_reps << " rep at " << reduced_weight << "lb -> " << (intended_reps / reduced_rm) * closest_weight->second << " rep at " << closest_weight->first << "lb]\n\n";
    
            // std::cout << "[Erasing " << closest_weight->first << "lb from Available Weights]\n";
            // weight_and_rep.erase(closest_weight);            
        }

    };
                
    std::vector<std::pair<long double, long double>> warmup_sets;
    warmup_sets.reserve(5);
    
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

    std::cout << "\nEstimated 1RM based on last performance: " << max_weight << "lb\n" << std::setprecision(precision_size);

    for (size_t j = 0; (j < warmup_sets.size()) && (warmup_sets[j].second >= 0.5); ++j)
    {
        std::cout << "\nWarmup " << j + 1    << "    : " << std::setw(largest_weight_length) << warmup_sets[j].first << "lb for " << unsigned(std::round(warmup_sets[j].second));
    }

    std::cout << "\n\nWorking set : " << std::setw(largest_weight_length) << target.weight << "lb for " << unsigned(target.reps) << '\n';

    return 0;
}
