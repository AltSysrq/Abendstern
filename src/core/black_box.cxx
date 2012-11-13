#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef DEBUG
#include <cstdio>
#include <cstring>
#include <map>
#include <queue>
#include <string>
#include <cstdarg>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>

#define TICKS_PER_SEC 1000000

static inline unsigned long long now() {
  timeval tv;
  gettimeofday(&tv, NULL);

  unsigned long long sec = tv.tv_sec;
  unsigned long long us = tv.tv_usec;
  sec *= TICKS_PER_SEC;
  return sec+us;
}

#define NOW_METH "wall time"

#else
#include <ctime>

#define TICKS_PER_SEC CLOCKS_PER_SEC
static inline unsigned long long now() {
  return (unsigned long long)std::clock();
}

#define NOW_METH "approximate CPU time"

#endif

#include "black_box.hxx"

using namespace std;

struct Entry {
  unsigned long long when;
  string what;
  unsigned nestingDepth;
};

static bool operator<(const Entry& a, const Entry& b) {
  return a.when > b.when;
}

struct Section {
  priority_queue<Entry> entries;
  unsigned nestingDepth;

  Section() : nestingDepth(0) {}
};

static map<string, Section> blackBoxSections;

BlackBox::BlackBox(const char* secs,
                   const char* format,
                   ...)
: sections(secs)
{
  unsigned long long now = ::now();
  char descr[1024];

  va_list args;
  va_start(args, format);

  vsnprintf(descr, sizeof(descr), format, args);
  va_end(args);

  //Copy descr to a std::string so that multiple sections may share the memory
  string sdescr(descr);
  //Add brace to indicate nesting
  sdescr += " {";

  while (*secs) {
    string sec(secs);
    secs += 1 + sec.size();

    Section& s(blackBoxSections[sec]);
    Entry e;
    e.when = now;
    e.what = sdescr;
    e.nestingDepth = s.nestingDepth;
    s.entries.push(e);

    ++s.nestingDepth;
  }
}

BlackBox::~BlackBox() {
  static const string descr("}");
  unsigned long long now = ::now();

  for (const char* section = sections; *section; section += 1+strlen(section)) {
    Section& s(blackBoxSections[string(section)]);
    Entry e;
    --s.nestingDepth;
    e.when = now;
    e.what = descr;
    e.nestingDepth = s.nestingDepth;
    s.entries.push(e);

    //If nesting depth is zero, prune the queue
    unsigned long long pruneBefore = now - 10*TICKS_PER_SEC;
    if (0 == s.nestingDepth) {
      while (s.entries.top().when < pruneBefore)
        s.entries.pop();
    }
  }
}

void BlackBox::dump(const char* section) {
  unsigned long long now = ::now();
  fprintf(stderr, "BEGIN: BlackBox information dump.\n");
  fprintf(stderr, "Times represent " NOW_METH " sampled at %d Hz",
          TICKS_PER_SEC);
  for (; *section; section += 1 + strlen(section)) {
    fprintf(stderr, "  BEGIN: BlackBox section %s\n", section);

    Section& s(blackBoxSections[string(section)]);
    while (!s.entries.empty()) {
      fprintf(stderr, "    %16lld %*s %s\n",
              now - s.entries.top().when,
              2*s.entries.top().nestingDepth, "",
              s.entries.top().what.c_str());
      s.entries.pop();
    }

    fprintf(stderr, "  END: BlackBox section %s\n", section);
  }
  fprintf(stderr, "END: BlackBox information dump.\n");
}

#endif /* DEBUG */
