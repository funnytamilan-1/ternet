#include "ternet.hpp"
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

namespace fs = std::filesystem;

static fs::path manifest_path(const std::string& dir) { return fs::path(dir) / "ternet.toml"; }
static fs::path lock_path(const std::string& dir) { return fs::path(dir) / "ternet.lock"; }

static std::string trim(std::string s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
    std::size_t i = 0;
    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
    s.erase(0, i);
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') s = s.substr(1, s.size() - 2);
    return s;
}

static std::map<std::string, std::string> dependencies(const fs::path& path) {
    std::ifstream f(path);
    if (!f) throw ternet::RuntimeError("cannot open manifest '" + path.string() + "'");
    std::map<std::string, std::string> result;
    std::string line;
    bool in_deps = false;
    while (std::getline(f, line)) {
        line = trim(line);
        if (line == "[dependencies]") { in_deps = true; continue; }
        if (!line.empty() && line.front() == '[') { in_deps = false; continue; }
        if (!in_deps || line.empty() || line.front() == '#') continue;
        const auto eq = line.find('=');
        if (eq == std::string::npos) throw ternet::RuntimeError("invalid dependency entry in '" + path.string() + "'");
        const auto name = trim(line.substr(0, eq));
        const auto spec = trim(line.substr(eq + 1));
        if (name.empty() || spec.empty()) throw ternet::RuntimeError("invalid dependency entry in '" + path.string() + "'");
        result[name] = spec;
    }
    return result;
}

static void require_manifest(const std::string& dir) {
    if (!fs::is_regular_file(manifest_path(dir)))
        throw ternet::RuntimeError("ternet.toml not found; run 'tnc init'");
}

int ternet::command_init(const std::string& dir) {
    fs::create_directories(fs::path(dir) / "src");
    const auto manifest = manifest_path(dir);
    if (fs::exists(manifest)) throw RuntimeError("ternet.toml already exists");
    std::ofstream f(manifest);
    f << "[package]\nname = \"my_app\"\nversion = \"0.1.0\"\nedition = \"2026\"\nentry = \"Node.trn\"\n\n[dependencies]\n";
    std::ofstream n(fs::path(dir) / "Node.trn");
    n << "tnprint(\"Hello, Ternet\"):\n";
    std::ofstream s(fs::path(dir) / "src/main.trn");
    s << "// Secondary source entry.\n";
    std::cout << "Created Ternet project in " << dir << "\nEntry: Node.trn\n";
    return 0;
}

int ternet::command_add(const std::string& dir, const std::string& name, const std::string& version) {
    require_manifest(dir);
    if (name.empty() || version.empty()) throw RuntimeError("package name and version are required");
    std::ifstream in(manifest_path(dir));
    std::stringstream b; b << in.rdbuf();
    std::string text = b.str();
    if (text.find("[dependencies]") == std::string::npos) text += "\n[dependencies]\n";

    std::istringstream lines(text);
    std::ostringstream rebuilt;
    std::string line;
    bool in_deps = false, replaced = false;
    while (std::getline(lines, line)) {
        const auto stripped = trim(line);
        if (stripped == "[dependencies]") { in_deps = true; rebuilt << line << '\n'; continue; }
        if (!stripped.empty() && stripped.front() == '[') in_deps = false;
        if (in_deps && stripped.rfind(name + " =", 0) == 0) {
            rebuilt << name << " = \"" << version << "\"\n";
            replaced = true;
        } else rebuilt << line << '\n';
    }
    if (!replaced) rebuilt << name << " = \"" << version << "\"\n";
    std::ofstream out(manifest_path(dir)); out << rebuilt.str();
    std::cout << "Added dependency " << name << " = " << version << "\n";
    return 0;
}

int ternet::command_install(const std::string& dir) {
    require_manifest(dir);
    const auto ds = dependencies(manifest_path(dir));
    const auto packages = fs::path(dir) / ".ternet" / "packages";
    fs::create_directories(packages);

    // v0.3 foundation: deterministic dependency metadata and lockfile.
    // Remote registry fetching is deliberately not claimed until implemented.
    std::ofstream lock(lock_path(dir));
    lock << "# Ternet lockfile v1\n[packages]\n";
    for (const auto& [name, spec] : ds) {
        lock << name << " = \"" << spec << "\"\n";
        const auto package_dir = packages / name;
        fs::create_directories(package_dir);
        std::ofstream metadata(package_dir / "package.toml");
        metadata << "name = \"" << name << "\"\nspec = \"" << spec << "\"\n";
    }
    std::cout << "Resolved " << ds.size() << " package(s); lockfile written to " << lock_path(dir) << "\n";
    return 0;
}

int ternet::command_remove(const std::string& dir, const std::string& name) {
    require_manifest(dir);
    std::ifstream in(manifest_path(dir));
    std::stringstream b; b << in.rdbuf();
    std::istringstream lines(b.str());
    std::ostringstream rebuilt;
    std::string line;
    bool in_deps = false, removed = false;
    while (std::getline(lines, line)) {
        const auto stripped = trim(line);
        if (stripped == "[dependencies]") { in_deps = true; rebuilt << line << '\n'; continue; }
        if (!stripped.empty() && stripped.front() == '[') in_deps = false;
        if (in_deps && stripped.rfind(name + " =", 0) == 0) { removed = true; continue; }
        rebuilt << line << '\n';
    }
    if (!removed) return 1;
    std::ofstream out(manifest_path(dir)); out << rebuilt.str();
    fs::remove_all(fs::path(dir) / ".ternet" / "packages" / name);
    std::cout << "Removed dependency " << name << "\n";
    return 0;
}

int ternet::command_list(const std::string& dir) {
    require_manifest(dir);
    const auto ds = dependencies(manifest_path(dir));
    for (const auto& [name, spec] : ds) std::cout << name << " = " << spec << "\n";
    if (ds.empty()) std::cout << "No dependencies.\n";
    return 0;
}

int ternet::command_package(const std::string& dir) {
    require_manifest(dir);
    const auto out = fs::path(dir) / ".ternet" / "package";
    fs::create_directories(out);
    std::ofstream manifest(out / "manifest.txt");
    manifest << "Ternet package manifest v1\n";
    for (const auto& entry : fs::recursive_directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const auto rel = fs::relative(entry.path(), dir);
        const auto first = rel.begin();
        if (first != rel.end() && (first->string() == ".git" || first->string() == ".ternet" || first->string() == "build")) continue;
        manifest << rel.generic_string() << "\n";
    }
    std::cout << "Package manifest created at " << out << "\n";
    return 0;
}
