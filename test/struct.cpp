const unsigned int bufSize = 20;

struct Foo {
    Foo(int n, double d, char c) : _n(n), _d(d) {
        for (int i = 0; i < bufSize; i++) {
            _buf[i] = c;
        }
    }

    int _n;
    double _d;
    char _buf[bufSize];
};

int main() {
    Foo *f = new Foo(42, 3.14, 'a');
    delete f;
    *f = Foo(42, 3.14, 'a');
    return 0;
}
