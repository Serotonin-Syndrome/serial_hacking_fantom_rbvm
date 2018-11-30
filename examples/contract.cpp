/*
    This is an example of a smart contract.
    We also provide a UI on our web site to deploy it.

    Moreover it contains an implementation of a hash table.


    This complex example embodies many features of programming languages.
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <new>
#include <utility>

#define smart_contract

template<class T> static
T *
alloc_for(size_t n)
{
    void *p = malloc(sizeof(T) * n);
    if (n && !p) {
        puts("Out of memory.");
        exit(1);
    }
    return static_cast<T *>(p);
}

template<class T, class ...Args>
T *
create(Args&& ...args)
{
    T *ptr = alloc_for<T>(1);
    new (ptr) T(std::forward<Args>(args)...);
    return ptr;
}

class Hashtable
{
    static uintptr_t zalloc8_(size_t n)
    {
        uint64_t *p = alloc_for<uint64_t>(n);
        for (size_t i = 0; i < n; ++i) {
            p[i] = 0;
        }
        return reinterpret_cast<uintptr_t>(p);
    }

    uint64_t ndata_;
    uintptr_t keys_;
    uintptr_t values_;

    uint64_t *
    get_keys_ptr_() const
    {
        return reinterpret_cast<uint64_t *>(keys_);
    }

    uint64_t *
    get_values_ptr_() const
    {
        return reinterpret_cast<uint64_t *>(values_);
    }
public:

    Hashtable(size_t nreserve = 1024)
        : ndata_(nreserve)
        , keys_(zalloc8_(ndata_))
        , values_(zalloc8_(ndata_))
    {}

    void
    insert(uint64_t k, uint64_t v)
    {
        uint64_t *keys = get_keys_ptr_();
        uint64_t *values = get_values_ptr_();
        uint64_t i = k % ndata_;
        while (keys[i] != k) {
            if (!keys[i]) {
                keys[i] = k;
                break;
            }
            i = (i + 1) % ndata_;
        }
        values[i] = v;
    }

    uint64_t
    get(uint64_t k) const
    {
        uint64_t *keys = get_keys_ptr_();
        uint64_t *values = get_values_ptr_();
        uint64_t i = k % ndata_;
        while (keys[i]) {
            if (keys[i] == k) {
                return values[i];
            }
            i = (i + 1) % ndata_;
        }
        return 0;
    }

    ~Hashtable()
    {
        free(get_keys_ptr_());
        free(get_values_ptr_());
    }
};

typedef uint64_t Address;

class ERC20
{
    uint64_t totalSupply() { return 0; }
    uint64_t balanceOf(Address who) { return 0; }
    bool transfer(Address from, Address to, uint64_t value) { return false; }
};

class MyToken : public ERC20
{
    uintptr_t balances_;
    uint64_t totalSupply_;

    Hashtable *
    get_balances_() const
    {
        return reinterpret_cast<Hashtable *>(balances_);
    }

public:
    MyToken(Address creator, uint64_t initial_balance)
        : balances_(reinterpret_cast<uintptr_t>(create<Hashtable>()))
        , totalSupply_(initial_balance)
    {
        get_balances_()->insert(creator, initial_balance);
    }

    uint64_t totalSupply() const
    {
        return totalSupply_;
    }

    uint64_t balanceOf(Address who) const
    {
        return get_balances_()->get(who);
    }

    bool transfer(Address from, Address to, uint64_t value)
    {
        Hashtable *b = get_balances_();
        if (!to) {
            return false;
        }

        const uint64_t prev_from_amount = b->get(from);
        if (prev_from_amount < value) {
            return false;
        }

        const uint64_t prev_to_amount = b->get(to);
        if (UINT64_MAX - prev_to_amount < value) {
            return false;
        }

        b->insert(from, prev_from_amount - value);
        b->insert(to, prev_to_amount + value);
        return true;
    }

    ~MyToken()
    {
        get_balances_()->~Hashtable();
    }
};

static
bool
eq_str(const char *a, const char *b)
{
    for (; *a == *b; ++a, ++b) {
        if (!*a) {
            return true;
        }
    }
    return false;
}

int
main()
{
    MyToken *mt = create<MyToken>(123, 1000000000);

#ifdef JUDGE
    printf("%d\n", (int) mt->balanceOf(123));
    printf("%d\n", (int) mt->balanceOf(456));

    printf("%s\n", mt->transfer(123, 456, 5) ? "Transferred OK" : "Failed to transfer");
    printf("%d\n", (int) mt->balanceOf(123));
    printf("%d\n", (int) mt->balanceOf(456));

    printf("%s\n", mt->transfer(123, 456, 1000000000) ? "Transferred OK" : "Failed to transfer");
    printf("%d\n", (int) mt->balanceOf(123));
    printf("%d\n", (int) mt->balanceOf(456));
#else

    // Possible commands:
    //     total-supply
    //     balance-of <address>
    //     transfer <from:address> <to:address> <amount>

    char *buf = alloc_for<char>(1024);
    while (scanf("%1023s", buf) == 1) {
        if (eq_str(buf, "total-supply")) {
            printf("%lu\n", static_cast<unsigned long>(mt->totalSupply()));
        } else if (eq_str(buf, "balance-of")) {
            unsigned long who;
            if (scanf("%lu", &who) != 1) {
                puts("Cannot read number!");
                continue;
            }
            printf("%lu\n", static_cast<unsigned long>(mt->balanceOf(who)));
        } else if (eq_str(buf, "transfer")) {
            unsigned long from, to, amount;
            if (scanf("%lu%lu%lu", &from, &to, &amount) != 3) {
                puts("Cannot read numbers!");
                continue;
            }
            if (mt->transfer(from, to, amount)) {
                puts("OK");
            } else {
                puts("Transfer failed!");
            }
        } else {
            puts("Command was not understood.");
        }
    }
#endif
}
