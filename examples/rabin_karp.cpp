#include <parlay/primitives.h>
#include <parlay/io.h>

// **************************************************************
// Rabin-Karp string matching.
// Generates a running hash such that the difference been two
// positions gives a hash for the string in between.  The search
// string can then be compared with the n - m pairs of strings 
// that differ by the length of the search string m.
// **************************************************************

// a finite field modulo a prime
struct field {
  static constexpr unsigned int p = 1045678717;
  using l = unsigned long;
  unsigned int val;
  template <typename Int>
  field(Int i) : val(i) {}
  field() {}
  field operator+(field a) {return field(((l) val + (l) a.val)%p);}
  field operator*(field a) {return field(((l) val * (l) a.val)%p);}
  bool operator==(field a) {return val == a.val;}
};
auto multm = parlay::monoid([] (field a, field b) {return a*b;}, field(1));

// works on any range type (e.g. std:vector, parlay::sequence)
// elements must be of integer type (e.g. char, int)
template <typename Range1, typename Range2>
long rabin_karp(const Range1& s, const Range2& str) {
  long n = s.size();
  long m = str.size();
  field x(500000000);

  // calculate hashes for prefixes of s
  auto xs = parlay::delayed_tabulate(n, [&] (long i) {
	      return x;});
  auto [powers, total] = parlay::scan(xs, multm);
  auto terms = parlay::tabulate(n, [&] (long i) {
		 return field(s[i]) * powers[i];});
  auto [hashes, sum] = parlay::scan(terms);
  
  // calculate hash for str
  auto terms2 = parlay::delayed_tabulate(m, [&] (long i) {
		  return field(str[i]) * powers[i];});
  field hash = parlay::reduce(terms2);
  
  // find matches
  auto y = parlay::delayed_tabulate(n-m+1, [&] (long i) {
	     field hash_end = (i == n - m) ? total: hashes[i+m];
	     if (hash * powers[i] + hashes[i] == hash_end &&
		 parlay::equal(str, s.cut(i,i+m))) // double check
	       return i;
	     return n; });
  return parlay::reduce(y, parlay::minm<long>());
}

// **************************************************************
// Driver code
// **************************************************************
using charseq = parlay::sequence<char>;

int main(int argc, char* argv[]) {
  auto usage = "Usage: rabin_karp <search_string> <filename>";
  if (argc != 3) std::cout << usage << std::endl;
  else {
    charseq str = parlay::chars_from_file(argv[2]);
    charseq search_str = parlay::to_chars(argv[1]);
    long loc = rabin_karp(str, search_str);
    if (loc < str.size())  
      std::cout << "found at position: " << loc << std::endl;
    else std::cout << "not found" << std::endl;
  }
}
