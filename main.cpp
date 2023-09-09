#include <iostream>
#include <httplib.h>
#include <dotenv.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <fstream>

using namespace std;

using json = nlohmann::json;

struct WatchlistItem
{
    string url;
    string route;
    string endpoint;
    chrono::milliseconds timeout;
    json data;
};

class Watchlist
{
    public:
        vector<WatchlistItem> items;

        Watchlist(vector<json> watchlist)
        {
            for (auto &item : watchlist)
            {
                WatchlistItem watchlist_item;
                watchlist_item.url = item["url"];
                watchlist_item.route = item["route"];
                watchlist_item.endpoint = item["endpoint"];
                watchlist_item.timeout = chrono::milliseconds(item["timeout"]);
                watchlist_item.data = NULL;
                items.push_back(watchlist_item);
            }
        }
};

int main()
{
    httplib::Server srv;
    dotenv::init();

    ifstream bridge_file("bridge.json");
    if (!bridge_file.good())
    {
        ofstream bridge_file("bridge.json");
        json example_watchlist;
        example_watchlist[0] = {
            {"url", "https://example.com/"},
            {"route", "/some?token=somevalue"},
            {"endpoint", "/test"},
            {"timeout", 20000}
        };
        example_watchlist[1] = {
            {"url", "https://example2.com/"},
            {"route", "/some2?token=somevalue"},
            {"endpoint", "/test2"},
            {"timeout", 20000}
        };
        bridge_file << example_watchlist.dump(4) << endl;

        cout << "Please fill bridge.json with your watchlist" << endl;
        return 1;
    }

    json bridge_json = json::parse(bridge_file);

    Watchlist watchlist(bridge_json);

    for (auto &item : watchlist.items)
    {
        thread t([&item]() {
            while (true)
            {
                this_thread::sleep_for(chrono::milliseconds(item.timeout));
                cout << "Updating data for " << item.url << endl;
                httplib::Client cli(item.url.c_str());
                auto cres = cli.Get(item.route.c_str());
                if (!cres)
                {
                    cout << "Error: " << cres.error() << endl;
                    continue;
                }
                item.data = json::parse(cres->body);
            }
        });
        t.detach();

        srv.Get(item.endpoint.c_str(), [&item](const httplib::Request&, httplib::Response &res) {
            if (item.data == NULL)
            {
                res.status = 418;
                return;
            }
            res.set_content(item.data.dump(), "application/json");
        });
    }

    srv.set_logger([](const httplib::Request &req, const httplib::Response &res) {
        cout << req.method << " " << req.path << " -> " << res.status << " [" << req.remote_addr << "]" << endl;
    });

    srv.Get("/", [](const httplib::Request&, httplib::Response &res) {
        res.set_content("Sync Bridge", "text/plain");
    });

    cout << "\nSync Bridge (" << getenv("HOST") << ":" << getenv("PORT") << ")\n";
    cout << "-----------------------------\n";
    for (auto &item : watchlist.items)
    {
        cout << item.endpoint << " [" << item.timeout.count() << "ms]  <-  " << item.url << "\n";
    }
    cout << endl;

    srv.listen(getenv("HOST"), stoi(getenv("PORT")));
    return 0;
}