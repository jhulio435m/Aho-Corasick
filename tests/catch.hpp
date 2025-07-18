#ifndef CATCH_HPP
#define CATCH_HPP

#include <exception>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace Catch {
struct TestCase { std::string name; std::function<void()> func; };
inline std::vector<TestCase>& registry() { static std::vector<TestCase> tests; return tests; }
struct Registrar { Registrar(const std::string& n, std::function<void()> f){ registry().push_back({n,f}); } };
inline int run() {
    int failures = 0;
    for(const auto& tc : registry()) {
        try { tc.func(); std::cout << "[PASSED] " << tc.name << "\n"; }
        catch(const std::exception& e){ ++failures; std::cout << "[FAILED] "<<tc.name<<" - "<<e.what()<<"\n"; }
        catch(...){ ++failures; std::cout << "[FAILED] "<<tc.name<<" - unknown error\n"; }
    }
    std::cout << (registry().size()-failures) << "/" << registry().size() << " tests passed\n";
    return failures;
}
} // namespace Catch

#define TEST_CASE(name) \
    static void name(); \
    static Catch::Registrar registrar_##name(#name, name); \
    static void name()

#define REQUIRE(cond) do { if(!(cond)) throw std::runtime_error("Requirement failed: " #cond); } while(0)

#endif // CATCH_HPP
