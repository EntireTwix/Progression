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

    // change weight while keeping relative intensity constant i.e 
    void update_weight(long double new_weight) noexcept
    {
        long double most_reps = this->reps + this->rir;

        Performance temp(this->complete(new_weight));
        long double temp_most_reps = temp.reps;
        temp.reps *= this->reps / most_reps;
        temp.rir += (temp_most_reps - temp.reps);

        *this = temp;
    }

    // update rir while keeping reps constant
    void update_reps(long double new_reps) noexcept { *this = this->complete(nullptr, new_reps, this->rir); }
    
    // update reps while keeping rir constant
    void update_rir(long double new_rir) noexcept { *this = this->complete(nullptr, this->reps, new_rir); }

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
