#include <iostream>
#include <type_traits>

template <bool has_w, bool has_reps, bool has_rir = true, std::enable_if_t<(!has_w ^ !has_reps ^ !has_rir), bool> = true>
struct PartialPerformance {};

template <>
struct PartialPerformance<false, true, true>
{
    long double reps = 0;
    long double rir = 0;
};

template <>
struct PartialPerformance<true, false, true>
{
    long double weight = 0;
    long double rir = 0;
};

template <>
struct PartialPerformance<true, true, false>
{
    long double weight = 0;
    long double reps = 0;
};

class Performance
{
private:
    long double weight = 0;
    long double reps = 0;
    long double rir = 0;

public:
    Performance(long double init_weight, long double init_reps, long double init_rir = 0) noexcept : weight(init_weight), reps(init_reps), rir(init_rir) {}

    long double get_weight() const noexcept { return this->weight; }
    long double get_reps() const noexcept { return this->reps; }
    long double get_rir() const noexcept { return this->rir; }

    // change weight while keeping relative intensity constant
    void shift_weight(long double new_weight)
    {
        long double most_reps = this->reps + this->rir;

        Performance temp(this->complete(new_weight));
        long double temp_most_reps = temp.reps;
        temp.reps *= this->reps / most_reps;
        temp.rir += (temp_most_reps - temp.reps);

        *this = temp;
    }

    long double estimate_rm() const noexcept
    {
        long double rep_adj = this->reps + this->rir;
        long double max_weight;

        if (rep_adj <= 8)
        {
            max_weight = this->weight / (1.0278 - (0.0278 * rep_adj));
        }
        else if (rep_adj >= 10)
        {
            max_weight = this->weight * (1 + ((rep_adj) / 30.0));
        }
        else
        {
            max_weight = ((this->weight * (10 - rep_adj)) / (2 * (1.0278 - (0.0278 * rep_adj))) + this->weight + ((this->weight * (-75 + (8 * rep_adj))) / 15) - ((this->weight * rep_adj * (10 - rep_adj)) / 60));
        }

        return max_weight;
    }

    static long double percent_change(Performance a, Performance b) { return (b.estimate_rm() - a.estimate_rm()) / a.estimate_rm(); }

    template <bool has_w, bool has_reps, bool has_rir>
    Performance complete(const PartialPerformance<has_w, has_reps, has_rir>& p) const noexcept
    {
        if constexpr (!has_w)
        {
            return this->complete(nullptr, p.reps, p.rir);
        }
        else if constexpr (!has_reps)
        {
            return this->complete(p.weight, nullptr, p.rir);
        }
        else
        {
            return this->complete(p.weight, p.reps, nullptr);
        }
    }

    Performance complete(std::nullptr_t target_weight, long double target_reps, long double target_rir = 0) const noexcept
    {
        long double rep_adj = target_reps + target_rir;
        long double deduced_weight;
        long double brzycki_est = estimate_rm() * (1.0278 - (0.0278 * rep_adj));
        long double epley_est = (30 * estimate_rm()) / (30 + rep_adj);

        if (this->reps + this->rir == rep_adj)
        {
            deduced_weight = this->weight;
        } 
        else if (rep_adj <= 8)
        {
            deduced_weight = brzycki_est;
        }
        else if (rep_adj >= 10)
        {
            deduced_weight = epley_est;
        }
        else
        {
            deduced_weight = ((estimate_rm() * (1.0278 - (0.0278 * rep_adj))) * 0.5) + (((30 * estimate_rm()) / (30 + rep_adj)) * 0.5);
        }

        return Performance(deduced_weight, target_reps, target_rir);
    }

    Performance complete(long double target_weight, std::nullptr_t target_reps = nullptr, long double target_rir = 0) const noexcept
    {
        long double deduced_reps;
        long double brzycki_threshold = estimate_rm() * 0.8054;
        long double epley_threshold = estimate_rm() * 0.75;
        long double rev_brzycki_est = -(((target_weight / estimate_rm()) - 1.0278) / 0.0278) - target_rir;
        long double rev_epley_est = (((30 * estimate_rm()) / target_weight) - 30) - target_rir;

        if ((target_weight == this->weight) && (target_rir == this->rir))
        {
            deduced_reps = this->reps;
        }
        // The weight whereafter any heavier would be less than 8 reps
        else if (target_weight >= brzycki_threshold)
        {
            deduced_reps = rev_brzycki_est;
        }
        // The weight whereafter any lighter would be more than 10 reps
        else if (target_weight <= epley_threshold) 
        {
            deduced_reps = rev_epley_est;
        }
        else
        {       
            // Not an exact solution
            deduced_reps = ((target_weight / brzycki_threshold) - 1.226244224) / -0.02953801791;
        }
        
        return Performance(target_weight, deduced_reps, target_rir);
    }

    Performance complete(long double target_weight, long double target_reps, std::nullptr_t target_rir = nullptr) const noexcept
    {
        return Performance(target_weight, target_reps, this->complete(target_weight).get_reps() - target_reps);
    }
};

std::ostream& operator<<(std::ostream& os, Performance p) 
{ 
    return os << p.get_weight() << "lb × " << p.get_reps() << " with " << p.get_rir() << " RIR";
}

// -- new file --

#include <set>
#include <vector>

int main()
{
    Performance baseline(55, 10, 1.5);
    std::set<long double> my_weights{3.749,4.265,4.772,5.261,5.288,5.758,5.777,6.274,6.284,6.781,6.800,7.271,7.297,7.787,8.294,8.810,9.007,9.523,10.030,10.519,10.546,11.016,11.035,11.532,11.542,12.039,12.058,12.529,12.555,13.045,13.552,14.068,14.345,14.861,15.368,15.857,15.884,16.355,16.373,16.870,16.880,17.377,17.396,17.867,17.893,18.383,18.890,19.406,19.603,20.119,20.626,21.115,21.142,21.613,21.631,22.128,22.138,22.636,22.654,23.125,23.151,23.641,23.754,24.148,24.270,24.664,24.777,24.941,25.267,25.293,25.457,25.764,25.783,25.964,26.280,26.290,26.453,26.480,26.787,26.806,26.951,26.969,27.276,27.302,27.467,27.476,27.792,27.974,27.992,28.299,28.463,28.489,28.815,28.979,29.012,29.486,29.528,30.002,30.035,30.199,30.525,30.551,30.715,31.022,31.041,31.222,31.538,31.548,31.712,31.738,32.045,32.064,32.209,32.227,32.534,32.561,32.725,32.734,33.050,33.232,33.250,33.557,33.721,33.747,34.073,34.237,34.350,34.744,34.866,35.373,35.863,35.889,36.360,36.378,36.876,36.886,37.383,37.401,37.872,37.899,38.388,38.895,39.411,39.608,40.124,40.631,41.121,41.147,41.618,41.637,42.134,42.144,42.641,42.659,43.130,43.157,43.646,43.760,44.153,44.275,44.782,44.946,45.272,45.298,45.462,45.769,45.788,45.969,46.285,46.295,46.459,46.485,46.792,46.811,46.956,46.975,47.281,47.308,47.472,47.482,47.797,47.979,47.998,48.304,48.468,48.495,48.820,48.984,49.018,49.491,49.533,50.041,50.204,50.530,50.556,50.720,51.027,51.046,51.227,51.543,51.553,51.717,51.743,52.050,52.069,52.214,52.233,52.539,52.566,52.730,52.740,53.055,53.237,53.562,53.726,54.356,54.872,55.379,55.868,55.894,56.365,56.384,56.881,56.891,57.388,57.407,57.878,57.904,58.393,58.900,59.614,60.130,60.637,61.126,61.152,61.623,61.642,62.139,62.149,62.646,63.136,64.952,65.468,65.975,66.464,66.491,66.961,66.980,67.477,67.487,67.984,68.474,70.210,70.726,71.233,71.722,72.219};

    std::vector<Performance> warm_ups;

    for (auto w : std::vector<std::pair<long double, long double>>({{5, 40}, {5, 25}, {3, (147 / 11.0L) + 3}, {2, 8}, {1, 4}}))
    {
        Performance temp(baseline.complete(nullptr, w.first, w.second));
        temp.shift_weight(*my_weights.lower_bound(temp.get_weight()));
        warm_ups.push_back(temp);
    }

    for (auto p : warm_ups)
    {
        std::cout << p << '\n';
    }
    
    return 0;
}

