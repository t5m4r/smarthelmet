// General utils for smarthelmet project

// Source - https://stackoverflow.com/a
// Posted by Jeff T., modified by community. See post 'Timeline' for change history
// Retrieved 2025-11-30, License - CC BY-SA 3.0

// The trimming method comes from https://stackoverflow.com/a/1798170/1613961
/*
wstring trim(const std::wstring& str, std::wstring& newline = L"\r\n")
{
    const auto strBegin = str.find_first_not_of(newline);
    if (strBegin == std::string::npos)
        return L""; // no content

    const auto strEnd = str.find_last_not_of(newline);
    const auto strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}
*/

String stripHtmlTags(const char* html) {
  if (html == nullptr) return String();
  String out;
  bool inTag = false;
  for (size_t i = 0; html[i] != '\0'; ++i) {
    char c = html[i];
    if (c == '<') {
      inTag = true;
      continue;
    }
    if (c == '>') {
      inTag = false;
      continue;
    }
    if (!inTag) {
      out += c;
    }
  }
  // Optional: trim leading/trailing whitespace
  int start = 0;
  while (start < out.length() && isspace(out[start])) start++;
  int end = out.length() - 1;
  while (end >= start && isspace(out[end])) end--;
  if (start == 0 && end == out.length() - 1) return out;
  return out.substring(start, end + 1);
}

