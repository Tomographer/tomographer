
#ifndef TOMOGRAPHER_CONSTSTR_H
#define TOMOGRAPHER_CONSTSTR_H



namespace Tomographer {
namespace Tools {



/** \brief A \c constexpr string type.
 *
 * This type supports compile time indexing, range checking, startswith(), find(),
 * substr() and friends.
 *
 * Currently, I've found no way to do compile-time string concatenation. Please tell me if
 * you have a good idea.
 */
class conststr
{
  const char* _p;
  std::size_t _sz;
public:
  template<std::size_t N>
  constexpr conststr(const char(&a)[N]) : _p(a), _sz(N - 1) {}
  constexpr conststr(const char *a, std::size_t n) : _p(a), _sz(n) {}
 
  inline constexpr char operator[](std::size_t n) const
  {
    return n < _sz ? _p[n] : throw std::out_of_range("");
  }
  inline constexpr std::size_t size() const { return _sz; }

  inline constexpr bool is_in_range(std::size_t n) const
  {
    return n < _sz ? true : false;
  }
  inline constexpr bool check_range(std::size_t n, bool answer = true) const
  {
    return is_in_range(n) ? answer : throw std::out_of_range("");
  }

  inline constexpr std::size_t clamp_to_range(const std::size_t pos)
  {
    return pos >= _sz ? _sz-1 : pos;
  }

  inline constexpr bool startswith(const conststr& s, std::size_t StartOffset = 0, std::size_t S_I = 0) const {
    return ((S_I >= s.size())
	    ? true
	    : (StartOffset+S_I < size() && s[S_I] == operator[](StartOffset+S_I)
	       ? startswith(s, StartOffset, S_I+1)
	       : false)
	);
  }

  inline constexpr bool operator==(const conststr& other) {
    return startswith(other) && other.size() == size();
  }

  inline constexpr conststr substr(std::size_t pos, std::size_t count = std::string::npos) const {
    return conststr(_p+pos, (pos > size() || count > size() || pos+count>size()) ? (size()-pos) : count);
  }
  inline constexpr conststr substr_e(std::size_t pos, std::size_t end = std::string::npos) const {
    return conststr(_p+pos, (end>size()) ? (size()-pos) : end-pos);
  }

  inline constexpr std::size_t find(const conststr& s, std::size_t pos = 0,
				    std::size_t not_found = std::string::npos) const
  {
    return (!is_in_range(pos)
	    ? ( not_found )
	    : ( startswith(s, pos)
		? pos
		: (pos <= size()-s.size()
		   ? find(s, pos+1, not_found)
		   : not_found) ));
  }

  inline constexpr std::size_t rfind(const conststr& s, std::size_t pos = std::string::npos,
				     std::size_t not_found = std::string::npos) const
  {
    return ((s.size() > size())
	    ? ( not_found )
	    : ((pos > size()-s.size())
	       ? rfind(s, size()-s.size(), not_found)
	       : ( startswith(s, pos)
		   ? pos
		   : ((pos > s.size())
		      ? rfind(s, pos-1, not_found)
		      : not_found) )));
  }

  inline std::string to_string() const { return std::string(_p, _sz); }
};



} // namespace Tools
} // namespace Tomographer







#endif
