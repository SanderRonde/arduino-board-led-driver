#include "../include/globals.h"

#define MAX_PERF_SECTIONS 14
#define ARR_INDICES 500
#define CHECKS_BEFORE_PRINT 1

namespace Perf {
    typedef struct perf_array {
        int length;
        uint32_t items[ARR_INDICES];
    } perf_array_t;

    typedef struct perf_section {
        const char *name;
        int id;
        perf_array_t perf_array;
        uint32_t last_start;
    } perf_section_t;

    int last_section_id = 1;
    perf_section_t sections[MAX_PERF_SECTIONS];
    int check_index = 0;

    int section_define(const char *name) {
        int id = last_section_id;
        last_section_id++;

        perf_section_t *section = &sections[id];
        section->name = name;
        section->id = id;
        section->perf_array.length = 0;
        section->last_start = millis();

        return id;
    }

    void section_begin(int section_id) {
        perf_section_t *section = &sections[section_id];
        section->last_start = millis();
    }

    void section_end(int section_id) {
        perf_section_t *section = &sections[section_id];
        uint32_t time_diff = millis() - section->last_start;

        if (section->perf_array.length >= ARR_INDICES - 1 || check_index != 0) {
            return;
        }
        section->perf_array.items[section->perf_array.length] = time_diff;
        section->perf_array.length++;
    }

    void print_results() {
        Serial.println("Results:");
        for (int i = 1; i < last_section_id; i++) {
            perf_section_t *section = &sections[i];
            uint64_t total = 0;
            uint32_t high = 0;
            uint32_t low = UINT32_MAX;
            for (int j = 0; j < section->perf_array.length; j++) {
                uint32_t value = section->perf_array.items[j];
                if (value > high) {
                    high = value;
                }
                if (value < low) {
                    low = value;
                }
                total += value;
            }
            if (low == UINT32_MAX) continue;
            double avg = (double)total / (double)section->perf_array.length;
            Serial.print(section->name);
            Serial.println("\t:");

            Serial.print("\tMin: ");
            Serial.print(low);
            Serial.print(", ");
            Serial.print("Max: ");
            Serial.print(high);
            Serial.print(", ");
            Serial.print("Avg: ");
            Serial.println(avg);
        }
        Serial.println("\n\n\n\n");
    }

    void reset_arrays() {
        for (int i = 1; i < last_section_id; i++) {
            perf_section_t *section = &sections[i];
            section->perf_array.length = 0;
        }
    }

    void check_results() {
        for (int i = 1; i < last_section_id; i++) {
            perf_section_t *section = &sections[i];
            if (section->perf_array.length >= ARR_INDICES - 1) {
                print_results();
                reset_arrays();
                return;
            }
        }
        check_index++;
        if (check_index >= CHECKS_BEFORE_PRINT) {
            check_index = 0;
        }
    }
}  // namespace Perf