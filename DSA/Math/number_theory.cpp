/**
 * NUMBER THEORY & MATH UTILITIES
 * ================================
 * Essential math for competitive programming.
 *
 * Topics:
 *   - Sieve of Eratosthenes  O(n log log n)
 *   - GCD / LCM
 *   - Modular arithmetic + Fast Power  O(log n)
 *   - Modular inverse
 *   - nCr mod p
 *   - Prime factorization
 */

#include <bits/stdc++.h>
using namespace std;
typedef long long ll;
typedef unsigned long long ull;
const ll MOD = 1e9 + 7;

// ─────────────────────────────────────────────
// 1. SIEVE OF ERATOSTHENES  O(n log log n)
// ─────────────────────────────────────────────
vector<bool> sieve(int n) {
    vector<bool> is_prime(n + 1, true);
    is_prime[0] = is_prime[1] = false;
    for (int i = 2; i * i <= n; i++)
        if (is_prime[i])
            for (int j = i * i; j <= n; j += i)
                is_prime[j] = false;
    return is_prime;
}

// Get all primes up to n
vector<int> getPrimes(int n) {
    auto is_prime = sieve(n);
    vector<int> primes;
    for (int i = 2; i <= n; i++)
        if (is_prime[i]) primes.push_back(i);
    return primes;
}

// Linear sieve – also computes smallest prime factor  O(n)
vector<int> linearSieve(int n) {
    vector<int> spf(n + 1, 0);  // smallest prime factor
    vector<int> primes;
    for (int i = 2; i <= n; i++) {
        if (!spf[i]) { spf[i] = i; primes.push_back(i); }
        for (int j = 0; j < (int)primes.size() && primes[j] <= spf[i] && (ll)i * primes[j] <= n; j++)
            spf[i * primes[j]] = primes[j];
    }
    return spf;  // spf[i]=0 means i is prime (or use spf[i]==i)
}

// ─────────────────────────────────────────────
// 2. GCD / LCM
// ─────────────────────────────────────────────
ll gcd(ll a, ll b) { return b == 0 ? a : gcd(b, a % b); }
ll lcm(ll a, ll b) { return a / gcd(a, b) * b; }  // a/gcd first to avoid overflow

// Extended GCD: returns gcd, and finds x,y such that a*x + b*y = gcd(a,b)
ll extGcd(ll a, ll b, ll& x, ll& y) {
    if (b == 0) { x = 1; y = 0; return a; }
    ll x1, y1;
    ll g = extGcd(b, a % b, x1, y1);
    x = y1;
    y = x1 - (a / b) * y1;
    return g;
}

// ─────────────────────────────────────────────
// 3. MODULAR ARITHMETIC
// ─────────────────────────────────────────────

// Fast power (modular exponentiation)  O(log exp)
ll fastPow(ll base, ll exp, ll mod = MOD) {
    ll result = 1;
    base %= mod;
    while (exp > 0) {
        if (exp & 1) result = result * base % mod;
        base = base * base % mod;
        exp >>= 1;
    }
    return result;
}

// Modular inverse using Fermat's little theorem (mod must be prime)
ll modInverse(ll a, ll mod = MOD) {
    return fastPow(a, mod - 2, mod);
}

// Modular inverse using extended GCD (works for any coprime a, mod)
ll modInverseExtGcd(ll a, ll mod) {
    ll x, y;
    ll g = extGcd(a, mod, x, y);
    if (g != 1) return -1;  // inverse doesn't exist
    return (x % mod + mod) % mod;
}

// Safe modular operations
ll addMod(ll a, ll b, ll mod = MOD) { return (a + b) % mod; }
ll subMod(ll a, ll b, ll mod = MOD) { return (a - b + mod) % mod; }
ll mulMod(ll a, ll b, ll mod = MOD) { return a % mod * (b % mod) % mod; }
ll divMod(ll a, ll b, ll mod = MOD) { return mulMod(a, modInverse(b, mod), mod); }

// ─────────────────────────────────────────────
// 4. COMBINATIONS nCr mod p  O(n) precomputation
// ─────────────────────────────────────────────
struct Combinatorics {
    int n;
    vector<ll> fact, inv_fact;

    Combinatorics(int n, ll mod = MOD) : n(n), fact(n + 1), inv_fact(n + 1) {
        fact[0] = 1;
        for (int i = 1; i <= n; i++) fact[i] = fact[i-1] * i % mod;
        inv_fact[n] = fastPow(fact[n], mod - 2, mod);
        for (int i = n - 1; i >= 0; i--) inv_fact[i] = inv_fact[i+1] * (i+1) % mod;
    }

    ll C(int n, int r, ll mod = MOD) {
        if (r < 0 || r > n) return 0;
        return fact[n] % mod * inv_fact[r] % mod * inv_fact[n-r] % mod;
    }

    ll P(int n, int r, ll mod = MOD) {  // permutation nPr
        if (r < 0 || r > n) return 0;
        return fact[n] % mod * inv_fact[n-r] % mod;
    }
};

// ─────────────────────────────────────────────
// 5. PRIME FACTORIZATION  O(sqrt(n))
// ─────────────────────────────────────────────
map<ll, int> primeFactors(ll n) {
    map<ll, int> factors;
    for (ll p = 2; p * p <= n; p++) {
        while (n % p == 0) { factors[p]++; n /= p; }
    }
    if (n > 1) factors[n]++;
    return factors;
}

// Count divisors using prime factorization
// If n = p1^a1 * p2^a2 * ... then d(n) = (a1+1)*(a2+1)*...
ll countDivisors(ll n) {
    ll count = 1;
    for (ll p = 2; p * p <= n; p++) {
        int exp = 0;
        while (n % p == 0) { exp++; n /= p; }
        count *= (exp + 1);
    }
    if (n > 1) count *= 2;
    return count;
}

// Sum of divisors
ll sumDivisors(ll n) {
    ll sum = 1;
    for (ll p = 2; p * p <= n; p++) {
        if (n % p == 0) {
            ll pk = 1, term = 0;
            while (n % p == 0) { pk *= p; n /= p; term += pk; }
            sum *= (term + 1);  // geometric sum: 1 + p + p² + ... + pᵏ
        }
    }
    if (n > 1) sum *= (1 + n);
    return sum;
}

// ─────────────────────────────────────────────
// 6. EULER'S TOTIENT  O(sqrt(n))
// ─────────────────────────────────────────────
// phi(n) = count of integers in [1,n] coprime to n
ll eulerTotient(ll n) {
    ll result = n;
    for (ll p = 2; p * p <= n; p++) {
        if (n % p == 0) {
            while (n % p == 0) n /= p;
            result -= result / p;
        }
    }
    if (n > 1) result -= result / n;
    return result;
}

// ─────────────────────────────────────────────
// 7. MATRIX EXPONENTIATION  O(k³ log n)
// ─────────────────────────────────────────────
// Compute n-th Fibonacci in O(log n) using matrix power.
typedef vector<vector<ll>> Matrix;

Matrix matMul(const Matrix& A, const Matrix& B, ll mod) {
    int k = A.size();
    Matrix C(k, vector<ll>(k, 0));
    for (int i = 0; i < k; i++)
        for (int l = 0; l < k; l++)
            if (A[i][l])
                for (int j = 0; j < k; j++)
                    C[i][j] = (C[i][j] + A[i][l] * B[l][j]) % mod;
    return C;
}

Matrix matPow(Matrix A, ll exp, ll mod) {
    int k = A.size();
    Matrix result(k, vector<ll>(k, 0));
    for (int i = 0; i < k; i++) result[i][i] = 1;  // identity
    while (exp > 0) {
        if (exp & 1) result = matMul(result, A, mod);
        A = matMul(A, A, mod);
        exp >>= 1;
    }
    return result;
}

ll fibMatrix(ll n, ll mod = MOD) {
    if (n <= 1) return n;
    Matrix A = {{1, 1}, {1, 0}};
    auto R = matPow(A, n - 1, mod);
    return R[0][0];  // fib(n) is top-left after ^(n-1)
}

// ─────────────────────────────────────────────
// EXERCISES
// ─────────────────────────────────────────────
/*
 * [1] Count primes using Sieve. Given n, how many primes <= n?
 *
 * [2] Ugly numbers: numbers whose only prime factors are 2, 3, 5.
 *     Find n-th ugly number. (DP with 3 pointers)
 *
 * [3] Chinese Remainder Theorem (CRT):
 *     Find x such that x ≡ r1 (mod m1), x ≡ r2 (mod m2), ...
 *
 * [4] Count trailing zeros in n!
 *     = floor(n/5) + floor(n/25) + floor(n/125) + ...
 *
 * [5] Catalan numbers: C(n) = C(2n,n)/(n+1).
 *     Number of BSTs, valid parentheses, etc.
 */

// Exercise [4] – Trailing zeros in n!
int trailingZeros(int n) {
    int count = 0;
    for (long long p = 5; p <= n; p *= 5) count += n / p;
    return count;
}

// Exercise [5] – n-th Catalan number mod p
ll catalanNumber(int n, ll mod = MOD) {
    Combinatorics comb(2 * n);
    return comb.C(2*n, n) * modInverse(n + 1) % mod;
}

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    // Sieve
    auto primes = getPrimes(50);
    cout << "Primes up to 50: ";
    for (int p : primes) cout << p << " ";
    cout << "\n";  // 2 3 5 7 11 13 17 19 23 29 31 37 41 43 47

    // GCD / LCM
    cout << "gcd(48, 18) = " << gcd(48, 18) << "\n";  // 6
    cout << "lcm(4, 6) = " << lcm(4, 6) << "\n";       // 12

    // Extended GCD
    ll x, y;
    ll g = extGcd(35, 15, x, y);
    cout << "extGcd(35,15): g=" << g << " x=" << x << " y=" << y << "\n";
    // 35*x + 15*y = 5

    // Fast power
    cout << "2^10 mod 1e9+7 = " << fastPow(2, 10) << "\n";  // 1024
    cout << "2^30 mod 7 = " << fastPow(2, 30, 7) << "\n";   // 1

    // Modular inverse
    cout << "inv(3) mod 7 = " << modInverse(3, 7) << "\n";  // 5 (3*5=15≡1 mod7)

    // nCr mod p
    Combinatorics comb(20);
    cout << "C(10,3) = " << comb.C(10, 3) << "\n";  // 120
    cout << "C(20,10) = " << comb.C(20, 10) << "\n";  // 184756

    // Prime factorization
    auto factors = primeFactors(360);
    cout << "360 = ";
    for (auto [p, e] : factors) cout << p << "^" << e << " ";  // 2^3 3^2 5^1
    cout << "\n";
    cout << "Divisors of 360: " << countDivisors(360) << "\n";  // 24

    // Matrix Fibonacci
    cout << "fib(10) matrix: " << fibMatrix(10) << "\n";  // 55
    cout << "fib(50) matrix: " << fibMatrix(50) << "\n";  // 12586269025

    // Trailing zeros
    cout << "Trailing zeros in 25!: " << trailingZeros(25) << "\n";  // 6

    // Catalan
    cout << "Catalan(5) = " << catalanNumber(5) << "\n";  // 42

    return 0;
}
