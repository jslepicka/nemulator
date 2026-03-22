export module callback;

export template <typename Derived> class i_callback
{
  protected:
    __forceinline Derived *derived() noexcept
    {
        //without this assume, the compiler generates null pointer checks before each
        //call, reducing performance
        __assume(this != nullptr);
        Derived *d = static_cast<Derived *>(this);
        return d;
    }
};