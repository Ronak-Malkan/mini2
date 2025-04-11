#ifndef VECTORIZED_DATASET_H
#define VECTORIZED_DATASET_H

#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <omp.h>
#include <iostream>

using namespace std;

class VectorizedDataSet {
public:
    vector<string> crash_date;
    vector<string> crash_time;
    vector<string> borough;
    vector<string> zip_code;
    vector<string> latitude;
    vector<string> longitude;
    vector<string> location;
    vector<string> on_street_name;
    vector<string> cross_street_name;
    vector<string> off_street_name;
    vector<int> number_of_persons_injured;
    vector<int> number_of_persons_killed;
    vector<int> number_of_pedestrians_injured;
    vector<int> number_of_pedestrians_killed;
    vector<int> number_of_cyclist_injured;
    vector<int> number_of_cyclist_killed;
    vector<int> number_of_motorist_injured;
    vector<int> number_of_motorist_killed;
    vector<string> contributing_factor_vehicle_1;
    vector<string> contributing_factor_vehicle_2;
    vector<string> contributing_factor_vehicle_3;
    vector<string> contributing_factor_vehicle_4;
    vector<string> contributing_factor_vehicle_5;
    vector<string> collision_id;
    vector<string> vehicle_type_code_1;
    vector<string> vehicle_type_code_2;
    vector<string> vehicle_type_code_3;
    vector<string> vehicle_type_code_4;
    vector<string> vehicle_type_code_5;

    // Count the number of lines (excluding header) in the CSV file.
    static size_t countLines(const string &filename) {
        ifstream file(filename);
        if (!file.is_open()) return 0;
        string line;
        size_t count = 0;
        // Skip header
        if(getline(file, line)) { }
        while(getline(file, line)) {
            count++;
        }
        return count;
    }

    // Load only a subset of lines from the file.
    // start: starting line index (0-based, after header) and count: number of lines to load.
    bool loadFromFileRange(const string &filename, size_t start, size_t count) {
        ifstream file(filename);
        if (!file.is_open()) return false;

        string header;
        if (!getline(file, header)) return false;

        // Skip lines until 'start'
        string line;
        for (size_t i = 0; i < start && getline(file, line); i++) { }
        
        size_t linesRead = 0;
        #pragma omp parallel
        {
            #pragma omp single nowait
            {
                while (linesRead < count && getline(file, line)) {
                    // Copy the line into a local variable for the task.
                    string localLine = line;
                    #pragma omp task firstprivate(localLine)
                    {
                        istringstream ss(localLine);
                        string token;
                        string cdate, ctime, brgh, zc, lat, lon, loc, on_str, cross_str, off_str;
                        int npi = 0, npk = 0, npi2 = 0, npk2 = 0, nci = 0, nck = 0, nmi = 0, nmk = 0;
                        string cfv1, cfv2, cfv3, cfv4, cfv5, cid;
                        string vtc1, vtc2, vtc3, vtc4, vtc5;

                        if(getline(ss, token, ',')) cdate = token;
                        if(getline(ss, token, ',')) ctime = token;
                        if(getline(ss, token, ',')) brgh = token;
                        if(getline(ss, token, ',')) zc = token;
                        if(getline(ss, token, ',')) lat = token;
                        if(getline(ss, token, ',')) lon = token;
                        if(getline(ss, token, ',')) loc = token;
                        if(getline(ss, token, ',')) on_str = token;
                        if(getline(ss, token, ',')) cross_str = token;
                        if(getline(ss, token, ',')) off_str = token;
                        if(getline(ss, token, ',')) { try { npi = stoi(token); } catch(...) {} }
                        if(getline(ss, token, ',')) { try { npk = stoi(token); } catch(...) {} }
                        if(getline(ss, token, ',')) { try { npi2 = stoi(token); } catch(...) {} }
                        if(getline(ss, token, ',')) { try { npk2 = stoi(token); } catch(...) {} }
                        if(getline(ss, token, ',')) { try { nci = stoi(token); } catch(...) {} }
                        if(getline(ss, token, ',')) { try { nck = stoi(token); } catch(...) {} }
                        if(getline(ss, token, ',')) { try { nmi = stoi(token); } catch(...) {} }
                        if(getline(ss, token, ',')) { try { nmk = stoi(token); } catch(...) {} }
                        if(getline(ss, token, ',')) cfv1 = token;
                        if(getline(ss, token, ',')) cfv2 = token;
                        if(getline(ss, token, ',')) cfv3 = token;
                        if(getline(ss, token, ',')) cfv4 = token;
                        if(getline(ss, token, ',')) cfv5 = token;
                        if(getline(ss, token, ',')) cid = token;
                        if(getline(ss, token, ',')) vtc1 = token;
                        if(getline(ss, token, ',')) vtc2 = token;
                        if(getline(ss, token, ',')) vtc3 = token;
                        if(getline(ss, token, ',')) vtc4 = token;
                        if(getline(ss, token, ',')) vtc5 = token;
                        
                        #pragma omp critical
                        {
                            crash_date.push_back(cdate);
                            crash_time.push_back(ctime);
                            borough.push_back(brgh);
                            zip_code.push_back(zc);
                            latitude.push_back(lat);
                            longitude.push_back(lon);
                            location.push_back(loc);
                            on_street_name.push_back(on_str);
                            cross_street_name.push_back(cross_str);
                            off_street_name.push_back(off_str);
                            number_of_persons_injured.push_back(npi);
                            number_of_persons_killed.push_back(npk);
                            number_of_pedestrians_injured.push_back(npi2);
                            number_of_pedestrians_killed.push_back(npk2);
                            number_of_cyclist_injured.push_back(nci);
                            number_of_cyclist_killed.push_back(nck);
                            number_of_motorist_injured.push_back(nmi);
                            number_of_motorist_killed.push_back(nmk);
                            contributing_factor_vehicle_1.push_back(cfv1);
                            contributing_factor_vehicle_2.push_back(cfv2);
                            contributing_factor_vehicle_3.push_back(cfv3);
                            contributing_factor_vehicle_4.push_back(cfv4);
                            contributing_factor_vehicle_5.push_back(cfv5);
                            collision_id.push_back(cid);
                            vehicle_type_code_1.push_back(vtc1);
                            vehicle_type_code_2.push_back(vtc2);
                            vehicle_type_code_3.push_back(vtc3);
                            vehicle_type_code_4.push_back(vtc4);
                            vehicle_type_code_5.push_back(vtc5);
                        }
                    } // end task
                    linesRead++;
                } // end while
            } // end single
            #pragma omp taskwait
        }
        file.close();
        return true;
    }

    // Search using parallel for with OpenMP; returns indices of records matching the condition.
    vector<size_t> searchByInjuryCountParallel(int minInjured) const {
        vector<size_t> indices;
        size_t n = number_of_persons_injured.size();
        #pragma omp parallel
        {
            vector<size_t> localIndices;
            #pragma omp for nowait
            for (size_t i = 0; i < n; i++) {
                if (number_of_persons_injured[i] >= minInjured)
                    localIndices.push_back(i);
            }
            #pragma omp critical
            {
                indices.insert(indices.end(), localIndices.begin(), localIndices.end());
            }
        }
        return indices;
    }

    static int get_num_threads_used() {
        int num_threads = 0;
        #pragma omp parallel
        {
            #pragma omp single
            num_threads = omp_get_num_threads();
        }
        return num_threads;
    }
};

#endif
