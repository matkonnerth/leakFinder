#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>

class Backtrace final
{
public:
    void addStack(const std::string& stackFrame)
    {
        m_frames.push_back(removeAdresses(stackFrame));
    }

    bool operator==(const Backtrace& other) const
    {
        return other.m_frames == m_frames;
    }

    const std::vector<std::string>& Frames() const
    {
        return m_frames;
    }

    void dump() const
    {
        for(const auto& frame : m_frames)
        {
            std::cout << frame << "\n";
        }
    }

    

    

private:
    std::string removeAdresses(const std::string& stackFrame) const
    {
        auto end = stackFrame.find("in");
        return stackFrame.substr(end+3);
    }
    std::vector<std::string> m_frames;

    
};

template <>
struct std::hash<Backtrace>
{
  std::size_t operator()(const Backtrace& bt) const
  {
    std::size_t ret = 0;
    for(auto& i : bt.Frames()) {
      ret ^= std::hash<std::string>()(i);
    }
    return ret;
  }
};

class Heap final
{
public:
    void alloc(const std::string& addr, Backtrace& bt)
    {     
        auto it = m_backtraces.find(bt);
        if(it!=m_backtraces.end())
        {
            
            it->second.push_back(addr);
            return;
        }
        std::vector<std::string> allocs{};
        allocs.push_back(addr);
        m_backtraces.insert(std::make_pair(bt, allocs));
        
    }

    void dealloc(const std::string& addr)
    {
        bool found = false;
        for(auto& bt : m_backtraces)
        {
            auto& btAllocs = bt.second;
            for(const auto& alloc : btAllocs)
            {
                if(alloc==addr)
                {
                    found = true;
                }
            }

            btAllocs.erase(std::remove_if(btAllocs.begin(), 
                              btAllocs.end(),
                              [&addr](const std::string& s) { return s==addr; }), btAllocs.end());
        }

       
        if(!found)
        {
            std::cout << "dealloc, addr: " << addr << " no allocation found" << "\n";
            m_deallocsNotFound.push_back(addr);
            return;
        }
        
    }

   
    void dump() const
    {
        std::cout << "heap dump" << "\n";
       
        for(const auto& bt : m_backtraces)
        {
            std::cout << "backtrace, alloc count: " << bt.second.size() << "\n";
            bt.first.dump();
            std::cout << "\n";
        }


        std::cout << "\n";
        for(const auto& notFoundDealloc: m_deallocsNotFound)
        {
            std::cout << notFoundDealloc << "\n";
        }
    }

    

private:
    
    std::unordered_map<Backtrace, std::vector<std::string>> m_backtraces;

    //
    std::vector<std::string> m_allocations{};
    std::vector<std::string> m_deallocsNotFound{};
};

int main(int argc, char* argv[])
{
    if(argc!=2)
    {
        std::cout << "input file missing" << "\n";
        return -1;
    }

    std::ifstream file(argv[1]);
    std::string line{};
    
    Heap heap{};

    Backtrace bt{};

    while(std::getline(file, line))
    {

        //std::cout << "process: " << line << "\n";
        if(line[0]=='$')
        {
            auto pos = line.find("0x");
            auto addr = line.substr(pos);
            heap.alloc(addr, bt);
        }
        else if(line.find("#0  __GI___libc_free")!=std::string::npos)
        {
            auto start = line.find("0x");
            auto end = line.find(")");
            auto addr = line.substr(start, end - start);
            heap.dealloc(addr);
            
        }
        else if(line[0]=='#')
        {
            bt.addStack(line);
        }
        //reset backtrace
        else if(line[0]=='[')
        {
            bt = Backtrace{};
        }        
        else
        {
            ;
        }
    }

    heap.dump();




}