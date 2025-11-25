#ifndef INVOKE_FORALL_H
#define INVOKE_FORALL_H

template <typename... Args> 
constexpr auto invoke_forall(Args&&... args)
{

}

template <typename T> 
constexpr decltype(auto) protect_arg(T&&)
{
    
}

#endif /* INVOKE_FORALL_H */
