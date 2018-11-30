static bool eq_str(const char *a, const char *b) {
    for (; *a == *b; ++a, ++b) {
        if (!*a) {
            return true;
        }
    }
    return false;
}

int main() {
    MyToken *mt = create<MyToken>(123, 1000000000);
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
}