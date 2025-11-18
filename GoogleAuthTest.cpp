
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define CPPHTTPLIB_NO_MMAP

#include <windows.h>
#include <random>
#include <string>
#include <unordered_set>
#include <sstream>
#include <iostream>
#include "httplib.h"

// ---- CONFIG: fill these in ----
static const char* GOOGLE_CLIENT_ID     = "YOUR_CLIENT_ID.apps.googleusercontent.com";
static const char* GOOGLE_CLIENT_SECRET = "YOUR_CLIENT_SECRET";
static const char* REDIRECT_URI         = "http://localhost:8080/auth/google/callback";
// scopes: OpenID Connect basic profile/email
static const char* SCOPE = "openid email profile";
// --------------------------------

static std::unordered_set<std::string> pending_states;

static std::string url_encode(const std::string &s) {
    std::ostringstream out;
    for (unsigned char c : s) {
        if (isalnum(c) || c=='-' || c=='_' || c=='.' || c=='~') out << c;
        else {
            out << '%' << std::uppercase << std::hex
                << (int)((c >> 4) & 0xF) << (int)(c & 0xF) << std::nouppercase << std::dec;
        }
    }
    return out.str();
}

static std::string random_state() {
    static std::mt19937_64 rng{std::random_device{}()};
    static std::uniform_int_distribution<uint64_t> dist;
    uint64_t v = dist(rng);
    std::ostringstream oss; oss << std::hex << v;
    return oss.str();
}

int main() {
    httplib::Server svr;

    // 1) Start OAuth: redirect the browser to Google
    svr.Get("/auth/google/start", [](const httplib::Request&, httplib::Response& res) {
        std::string state = random_state();
        pending_states.insert(state);

        std::string auth_url =
            std::string("https://accounts.google.com/o/oauth2/v2/auth?")
            + "response_type=code"
            + "&client_id=" + url_encode(GOOGLE_CLIENT_ID)
            + "&redirect_uri=" + url_encode(REDIRECT_URI)
            + "&scope=" + url_encode(SCOPE)
            + "&state=" + url_encode(state)
            + "&access_type=offline"
            + "&prompt=consent";

        res.set_redirect(auth_url.c_str()); // 302
    });

    // 2) Google redirects back here with ?code=...&state=...
    svr.Get("/auth/google/callback", [](const httplib::Request& req, httplib::Response& res) {
        auto it_code  = req.get_param_value("code");
        auto it_state = req.get_param_value("state");

        if (it_state.empty() || !pending_states.count(it_state)) {
            res.status = 400;
            res.set_content("Invalid or missing state.", "text/plain");
            return;
        }
        pending_states.erase(it_state);

        if (it_code.empty()) {
            res.status = 400;
            res.set_content("Missing 'code' parameter.", "text/plain");
            return;
        }

        // ---- Option A: stop here (you proved Google auth redirect works) ----
        // res.set_content("Auth code = " + it_code + "\nNow exchange it on the server.", "text/plain");
        // return;

        // ---- Option B: quick token exchange via system curl (works on Win10+) ----
        // POST to https://oauth2.googleapis.com/token with x-www-form-urlencoded body
        std::string post_body =
            std::string("code=") + url_encode(it_code) +
            "&client_id=" + url_encode(GOOGLE_CLIENT_ID) +
            "&client_secret=" + url_encode(GOOGLE_CLIENT_SECRET) +
            "&redirect_uri=" + url_encode(REDIRECT_URI) +
            "&grant_type=authorization_code";

        // Write body to a temp file to avoid escaping hell on Windows
        char tmpPath[MAX_PATH]; GetTempPathA(MAX_PATH, tmpPath);
        char tmpFile[MAX_PATH]; GetTempFileNameA(tmpPath, "tok", 0, tmpFile);
        {
            FILE* f = fopen(tmpFile, "wb");
            fwrite(post_body.data(), 1, post_body.size(), f);
            fclose(f);
        }

        // Use curl to post and capture response to another temp file
        char outFile[MAX_PATH]; GetTempFileNameA(tmpPath, "out", 0, outFile);
        std::ostringstream cmd;
        cmd << "curl -s -X POST \"https://oauth2.googleapis.com/token\" "
            << "--header \"Content-Type: application/x-www-form-urlencoded\" "
            << "--data-binary \"@" << tmpFile << "\" "
            << "-o \"" << outFile << "\"";

        int rc = system(cmd.str().c_str());

        std::string json;
        {
            FILE* f = fopen(outFile, "rb");
            if (f) {
                char buf[4096];
                size_t n;
                while ((n = fread(buf, 1, sizeof(buf), f)) > 0) json.append(buf, n);
                fclose(f);
            }
        }
        DeleteFileA(tmpFile);
        DeleteFileA(outFile);

        if (rc != 0 || json.empty()) {
            res.status = 500;
            res.set_content("Token exchange failed.\n", "text/plain");
            return;
        }

        // json contains: access_token, id_token (JWT), refresh_token (if offline), expires_in, token_type
        std::string html = "<h1>Token response</h1><pre>" + json + "</pre>";
        html += "<p>Parse the JSON, store tokens, and set your own session cookie.</p>";
        res.set_content(html, "text/html");
    });

    // Health check
    svr.Get("/", [](const httplib::Request&, httplib::Response& res){
        res.set_content("OK", "text/plain");
    });

    std::cout << "Listening on http://localhost:8080\n";
    svr.listen("0.0.0.0", 8080);
    return 0;
}
