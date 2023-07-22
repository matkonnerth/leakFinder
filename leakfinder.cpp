#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

class Backtrace final {
public:
  void addStack(const std::string &stackFrame) {
    m_frames.push_back(stackFrame);
  }

  bool operator==(const Backtrace &other) const {
    return other.m_frames == m_frames;
  }

  const std::vector<std::string> &Frames() const { return m_frames; }

  void dump() const {
    for (const auto &frame : m_frames) {
      std::cout << frame << "\n";
    }
  }

private:
  std::vector<std::string> m_frames;
};

template <> struct std::hash<Backtrace> {
  std::size_t operator()(const Backtrace &bt) const {
    std::size_t ret = 0;
    for (auto &i : bt.Frames()) {
      ret ^= std::hash<std::string>()(i);
    }
    return ret;
  }
};

class Heap final {
public:
  void alloc(const std::string &addr, Backtrace &bt) {
    auto it = m_backtraces.find(bt);
    if (it != m_backtraces.end()) {

      it->second.push_back(addr);
      return;
    }
    std::vector<std::string> allocs{};
    allocs.push_back(addr);
    m_backtraces.insert(std::make_pair(bt, allocs));
  }

  void dealloc(const std::string &addr) {
    bool found = false;
    for (auto &bt : m_backtraces) {
      auto &btAllocs = bt.second;
      for (const auto &alloc : btAllocs) {
        if (alloc == addr) {
          found = true;
        }
      }

      btAllocs.erase(
          std::remove_if(btAllocs.begin(), btAllocs.end(),
                         [&addr](const std::string &s) { return s == addr; }),
          btAllocs.end());
    }

    if (!found) {
      // std::cout << "dealloc, addr: " << addr << " no allocation found" <<
      // "\n";
      m_deallocsNotFound.push_back(addr);
      return;
    }
  }

  void dump() const {
    std::cout << "heap dump"
              << "\n";

    std::vector<std::pair<Backtrace, std::vector<std::string>>> backtraces{};
    for (const auto &bt : m_backtraces) {
      backtraces.push_back(bt);
    }

    std::sort(backtraces.begin(), backtraces.end(),
              [](std::pair<Backtrace, std::vector<std::string>> &a,
                 std::pair<Backtrace, std::vector<std::string>> &b) {
                return (a.second.size() < b.second.size());
              });

    for(const auto& b : backtraces)
    {
        std::cout << "backtrace, alloc count: " << b.second.size() << "\n";
        b.first.dump();
    }
  }

private:
  std::unordered_map<Backtrace, std::vector<std::string>> m_backtraces;

  //
  std::vector<std::string> m_allocations{};
  std::vector<std::string> m_deallocsNotFound{};
};

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cout << "input file missing"
              << "\n";
    return -1;
  }

  std::ifstream file(argv[1]);
  std::string line{};

  Heap heap{};

  Backtrace bt{};

  std::string mallocAddr{};

  int lineCnt = 0;

  while (std::getline(file, line)) {
    lineCnt++;
    if (lineCnt % 1000 == 0) {
      std::cout << "line: " << std::to_string(lineCnt) << "\n";
    }
    // std::cout << "process: " << line << "\n";
    if (line.find("#malloc") != std::string::npos) {
      if (!mallocAddr.empty()) {
        heap.alloc(mallocAddr, bt);
        mallocAddr = std::string{};
      }
      auto pos = line.find("0x");
      auto addr = line.substr(pos);

      mallocAddr = addr;

      // new Backtrace
      bt = Backtrace{};
    } else if (line.find("#free()") != std::string::npos) {
      if (!mallocAddr.empty()) {
        heap.alloc(mallocAddr, bt);
        mallocAddr = std::string{};
      }
      auto start = line.find("0x");
      if (start != std::string::npos) {
        auto addr = line.substr(start);
        heap.dealloc(addr);
      }

      // new Backtrace
      bt = Backtrace{};
    } else {
      bt.addStack(line);
    }
  }

  heap.dump();
}