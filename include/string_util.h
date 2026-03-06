#ifndef STRING_UTIL_H
#define STRING_UTIL_H

#include <string>
#include <cctype>

std::string create_slug(const std::string &input)
{
  std::string slug;
  bool last_was_hyphen = true; // Start true to prevent leading hyphens

  for (char c : input)
  {
    // Convert to lowercase and check if alphanumeric
    if (std::isalnum(static_cast<unsigned char>(c)))
    {
      slug += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
      last_was_hyphen = false;
    }
    // If not alphanumeric and we haven't just added a hyphen, add one
    else if (!last_was_hyphen)
    {
      slug += '-';
      last_was_hyphen = true;
    }
  }

  // Remove trailing hyphen if it exists
  if (!slug.empty() && slug.back() == '-')
  {
    slug.pop_back();
  }

  return slug;
}

#endif