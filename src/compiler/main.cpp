#include <nwtrees/Lexer.hpp>
#include <nwtrees/util/Assert.hpp>

#include <chrono>
#include <filesystem>

std::string read_source_file(const char* path)
{
    FILE* file = std::fopen(path, "rb");

    if (file)
    {
        std::fseek(file, 0, SEEK_END);
        std::size_t len = ftell(file);
        std::fseek(file, 0, SEEK_SET);

        std::string out;
        out.resize(len);

        size_t read = std::fread(out.data(), 1, len, file);
        NWTREES_ASSERT(read == len);

        std::fclose(file);
        return out;
    }

    return "";
}

int main(const int argc, const char** argv)
{
    std::filesystem::path folder = "D:/_nwn/_server_codebases";

    std::vector<std::filesystem::path> scripts_to_build;

    for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(folder))
    {
        if (std::filesystem::is_regular_file(entry) &&
            entry.path().extension() == ".nss")
        {
            scripts_to_build.push_back(entry.path());
        }
    }

    float total_time = 0.0f;

    nwtrees::LexerOutput lexer;

    for (const std::filesystem::path& path : scripts_to_build)
    {
        const std::string file = read_source_file(path.string().c_str());

        const auto before = std::chrono::high_resolution_clock::now();
        lexer = nwtrees::lexer(file.c_str(), std::move(lexer));
        const auto after = std::chrono::high_resolution_clock::now();

        const float ms = std::chrono::duration_cast<std::chrono::nanoseconds>(after - before).count() / 1000000.0f;
        total_time += ms;

        if (!lexer.errors.empty())
        {
            printf("ERROR: %d\n", lexer.errors[0].code);
            __debugbreak();
        }
    }

    printf("Total runtime: %.2f ms\n", total_time);
    fflush(stdout);
}
