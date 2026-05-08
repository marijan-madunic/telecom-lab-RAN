#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <random>
#include <vector>
#include <ctime>
#include <sstream>
#include <netinet/in.h>
#include <unistd.h>

int server_fd;

int ran_events_total = 0;
int measurement_reports = 0;
int handover_candidates = 0;

int last_rsrp = 0;
int last_rsrq = 0;
int last_sinr = 0;

std::string generate_metrics() {
    std::stringstream ss;

    ss << "# HELP ran_events_total Total RAN events\n";
    ss << "# TYPE ran_events_total counter\n";
    ss << "ran_events_total " << ran_events_total << "\n";

    ss << "# HELP ran_measurement_reports_total Total measurement reports\n";
    ss << "# TYPE ran_measurement_reports_total counter\n";
    ss << "ran_measurement_reports_total " << measurement_reports << "\n";

    ss << "# HELP ran_handover_candidates_total Total handover candidates\n";
    ss << "# TYPE ran_handover_candidates_total counter\n";
    ss << "ran_handover_candidates_total " << handover_candidates << "\n";

    ss << "# HELP ran_rsrp_dbm Last RSRP value\n";
    ss << "# TYPE ran_rsrp_dbm gauge\n";
    ss << "ran_rsrp_dbm " << last_rsrp << "\n";

    ss << "# HELP ran_rsrq_db Last RSRQ value\n";
    ss << "# TYPE ran_rsrq_db gauge\n";
    ss << "ran_rsrq_db " << last_rsrq << "\n";

    ss << "# HELP ran_sinr_db Last SINR value\n";
    ss << "# TYPE ran_sinr_db gauge\n";
    ss << "ran_sinr_db " << last_sinr << "\n";

    return ss.str();
}

void start_http_server() {
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8000);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);

    while (true) {
        int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);

        std::string response =
            "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n" +
            generate_metrics();

        send(new_socket, response.c_str(), response.size(), 0);
        close(new_socket);
    }
}

int main() {
    std::cout << "RAN Simulator started..." << std::endl;

    std::thread http_thread(start_http_server);

    std::vector<std::string> imsis = {
        "001010000000001",
        "001010000000002",
        "001010000000003",
        "001010000000004"
    };

    std::vector<std::string> cells = {
        "cell-001",
        "cell-002",
        "cell-003"
    };

    std::default_random_engine generator(time(nullptr));
    std::uniform_int_distribution<int> imsi_dist(0, imsis.size() - 1);
    std::uniform_int_distribution<int> cell_dist(0, cells.size() - 1);
    std::uniform_int_distribution<int> rsrp_dist(-115, -75);
    std::uniform_int_distribution<int> rsrq_dist(-20, -5);
    std::uniform_int_distribution<int> sinr_dist(0, 30);

    while (true) {
        std::string imsi = imsis[imsi_dist(generator)];
        std::string cell_id = cells[cell_dist(generator)];

        last_rsrp = rsrp_dist(generator);
        last_rsrq = rsrq_dist(generator);
        last_sinr = sinr_dist(generator);

        ran_events_total++;

        std::cout << "[RAN EVENT] imsi=" << imsi
                  << " cell_id=" << cell_id
                  << " rsrp=" << last_rsrp
                  << " rsrq=" << last_rsrq
                  << " sinr=" << last_sinr;

        if (last_rsrp < -105) {
            handover_candidates++;
            std::cout << " event=handover_candidate";
        } else {
            measurement_reports++;
            std::cout << " event=measurement_report";
        }

        std::cout << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(3));
    }

    http_thread.join();
    return 0;
}
