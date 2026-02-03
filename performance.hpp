#include <iostream>
#include <type_traits>
#include <cassert>
#include <cmath>

template <bool has_w, bool has_reps, bool has_rir, std::enable_if_t<(!has_w ^ !has_reps ^ !has_rir), bool> = true>
struct PartialPerformance {};

template <>
struct PartialPerformance<false, true, true>
{
    long double reps;
    long double rir;
};

template <>
struct PartialPerformance<true, false, true>
{
    long double weight;
    long double rir;
};

template <>
struct PartialPerformance<true, true, false>
{
    long double weight;
    long double reps;
};

class Performance
{
private:
    long double weight;
    long double reps;
    long double rir;

public:
    Performance(long double init_weight, long double init_reps, long double init_rir = 0) noexcept : weight(init_weight), reps(init_reps), rir(init_rir) {}

    long double get_weight() const noexcept { return this->weight; }
    long double get_reps() const noexcept { return this->reps; }
    long double get_rir() const noexcept { return this->rir; }

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
        // else
        // {
        //     return this->complete(p.weight, p.reps, nullptr);
        // }
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
            std::cout << brzycki_est << ' ' << epley_est << ' ' << Performance(deduced_weight, target_reps, target_rir).estimate_rm() << '\n';
        }

        return Performance(deduced_weight, target_reps, target_rir);
    }

    Performance complete(long double target_weight, std::nullptr_t target_reps, long double target_rir = 0) const noexcept
    {
        long double deduced_reps;
        long double brzycki_threshold = estimate_rm() * ((-0.278 * target_rir) + 0.8054);
        long double epley_threshold = (30 * estimate_rm()) / (target_rir + 40);
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
            // Gross result of ChatGPT usage because I haven't learned

            long double lo = 8, hi = 10;
            long double flo = Performance(target_weight, lo).estimate_rm() - estimate_rm();
            long double fhi = Performance(target_weight, hi).estimate_rm() - estimate_rm();

            if ((flo > 0.0L) == (fhi > 0.0L))
            {
                deduced_reps = (std::fabsl(flo) < std::fabsl(fhi)) ? lo : hi;
            }
            else
            {
                for (int i = 0; i < 100; ++i) 
                {
                    long double mid = (lo + hi) * 0.5L;
                    long double fmid = Performance(target_weight, mid).estimate_rm() - estimate_rm();
                
                    if (std::fabsl(fmid) < 1e-12L || (hi - lo) < 1e-12L)
                    {
                        deduced_reps = mid;
                        break;
                    }
                    
                    if ((flo > 0.0L) == (fmid > 0.0L)) 
                    {
                        lo = mid; 
                        flo = fmid;
                    } 
                    else 
                    {
                        hi = mid; 
                        fhi = fmid;
                    }
                }

                deduced_reps = (lo + hi) * 0.5L;
            }
        }
        
        return Performance(target_weight, deduced_reps, target_rir);
    }
};

std::ostream& operator<<(std::ostream& os, Performance p) 
{ 
    return os << p.get_weight() << "lb × " << p.get_reps() << " with " << p.get_rir() << " RIR";
}

int main()
{   
    Performance baseline(55, 10);
    std::cout << baseline.estimate_rm() << '\n';
    std::cout << baseline.complete(nullptr, 8) << '\n';
    std::cout << baseline.complete(nullptr, 9) << '\n';
    std::cout << baseline.complete(nullptr, 10) << '\n';
    std::cout << baseline.complete(59.0627, nullptr) << '\n';
    std::cout << baseline.complete(56.7171, nullptr) << '\n';
    std::cout << baseline.complete(55, nullptr) << '\n';

    return 0;
}

