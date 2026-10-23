#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <memory>
#include <algorithm>
#include <iomanip>
#include <limits>
#include <string>
#include <ctime>
#include <stdexcept>

using namespace std;

// Educational C++ OOP prototype. Not for real clinical allocation.
// Compatibility below uses simplified ABO/Rh rules for red-cell transfusion only.
// Real transfusion decisions must be made by qualified blood-bank staff.

string todayDate() {
    time_t now = time(nullptr);
    tm *ltm = localtime(&now);
    if (!ltm) return "1970-01-01";
    ostringstream out;
    out << (1900 + ltm->tm_year) << '-'
        << setw(2) << setfill('0') << (1 + ltm->tm_mon) << '-'
        << setw(2) << setfill('0') << ltm->tm_mday;
    return out.str();
}

int readInt(const string& prompt, int minValue, int maxValue) {
    while (true) {
        cout << prompt;
        string line;
        if (!getline(cin, line)) throw runtime_error("Input closed.");
        stringstream ss(line);
        int value;
        char extra;
        if ((ss >> value) && !(ss >> extra) && value >= minValue && value <= maxValue)
            return value;
        cout << "Please enter a number from " << minValue << " to " << maxValue << ".\n";
    }
}

string readText(const string& prompt) {
    cout << prompt;
    string value;
    getline(cin, value);
    if (value.empty()) throw invalid_argument("Input cannot be empty.");
    return value;
}

bool isValidBloodGroup(const string& group) {
    const vector<string> groups = {"O-", "O+", "A-", "A+", "B-", "B+", "AB-", "AB+"};
    return find(groups.begin(), groups.end(), group) != groups.end();
}

// Simplified compatibility: recipient group can receive red cells from donor group.
bool compatibleRedCells(const string& recipient, const string& donor) {
    if (recipient == "O-") return donor == "O-";
    if (recipient == "O+") return donor == "O-" || donor == "O+";
    if (recipient == "A-") return donor == "O-" || donor == "A-";
    if (recipient == "A+") return donor == "O-" || donor == "O+" || donor == "A-" || donor == "A+";
    if (recipient == "B-") return donor == "O-" || donor == "B-";
    if (recipient == "B+") return donor == "O-" || donor == "O+" || donor == "B-" || donor == "B+";
    if (recipient == "AB-") return donor == "O-" || donor == "A-" || donor == "B-" || donor == "AB-";
    if (recipient == "AB+") return isValidBloodGroup(donor);
    return false;
}

class Person {
protected:
    int id;
    string name;
public:
    Person(int id, string name) : id(id), name(std::move(name)) {}
    virtual ~Person() = default;
    int getId() const { return id; }
    string getName() const { return name; }
    virtual string role() const = 0; // Abstraction + polymorphism
};

class Donor : public Person {
    string bloodGroup;
    string phone;
    bool available;
public:
    Donor(int id, string name, string bloodGroup, string phone, bool available = true)
        : Person(id, std::move(name)), bloodGroup(std::move(bloodGroup)),
          phone(std::move(phone)), available(available) {
        if (!isValidBloodGroup(this->bloodGroup)) throw invalid_argument("Invalid blood group.");
    }
    string role() const override { return "Donor"; }
    string getBloodGroup() const { return bloodGroup; }
    string getPhone() const { return phone; }
    bool isAvailable() const { return available; }
    void setAvailable(bool value) { available = value; }
};

class Patient : public Person {
    string bloodGroup;
public:
    Patient(int id, string name, string bloodGroup)
        : Person(id, std::move(name)), bloodGroup(std::move(bloodGroup)) {
        if (!isValidBloodGroup(this->bloodGroup)) throw invalid_argument("Invalid blood group.");
    }
    string role() const override { return "Patient"; }
    string getBloodGroup() const { return bloodGroup; }
};

class BloodRequest {
    int requestId;
    string hospital;
    string patientName;
    string recipientGroup;
    int units;
    int urgency; // 1=Routine, 2=Urgent, 3=Critical
    string requestDate;
    bool fulfilled;
public:
    BloodRequest(int id, string hospital, string patient, string group, int units, int urgency)
        : requestId(id), hospital(std::move(hospital)), patientName(std::move(patient)),
          recipientGroup(std::move(group)), units(units), urgency(urgency),
          requestDate(todayDate()), fulfilled(false) {
        if (!isValidBloodGroup(recipientGroup)) throw invalid_argument("Invalid blood group.");
        if (units <= 0) throw invalid_argument("Units must be positive.");
        if (urgency < 1 || urgency > 3) throw invalid_argument("Urgency must be 1, 2, or 3.");
    }
    int getId() const { return requestId; }
    string getHospital() const { return hospital; }
    string getPatientName() const { return patientName; }
    string getGroup() const { return recipientGroup; }
    int getUnits() const { return units; }
    int getUrgency() const { return urgency; }
    string getDate() const { return requestDate; }
    bool isFulfilled() const { return fulfilled; }
    void markFulfilled() { fulfilled = true; }
};

class BloodUnit {
    int unitId;
    string group;
    string expiryDate; // ISO format YYYY-MM-DD for lexical date comparison
    bool allocated;
public:
    BloodUnit(int id, string group, string expiry)
        : unitId(id), group(std::move(group)), expiryDate(std::move(expiry)), allocated(false) {
        if (!isValidBloodGroup(this->group)) throw invalid_argument("Invalid blood group.");
        if (expiryDate.size() != 10 || expiryDate[4] != '-' || expiryDate[7] != '-')
            throw invalid_argument("Expiry date must be YYYY-MM-DD.");
    }
    int getId() const { return unitId; }
    string getGroup() const { return group; }
    string getExpiryDate() const { return expiryDate; }
    bool isAllocated() const { return allocated; }
    void allocate() { allocated = true; }
    bool isExpired() const { return expiryDate < todayDate(); }
};

class AllocationManager {
    vector<BloodUnit> units;
    vector<BloodRequest> requests;
    vector<string> history;
    int nextUnitId = 1;
    int nextRequestId = 1;

public:
    AllocationManager() { load(); }

    void addUnit(const string& group, const string& expiry) {
        units.emplace_back(nextUnitId++, group, expiry);
        save();
    }

    void addRequest(const string& hospital, const string& patient, const string& group,
                    int count, int urgency) {
        requests.emplace_back(nextRequestId++, hospital, patient, group, count, urgency);
        save();
    }

    void registerDonor(vector<shared_ptr<Person>>& people) {
        // Kept as a separate operation in main; donor data is also saved persistently.
        (void)people;
    }

    int availableUnitsFor(const string& recipientGroup) const {
        int count = 0;
        for (const auto& unit : units)
            if (!unit.isAllocated() && !unit.isExpired() &&
                compatibleRedCells(recipientGroup, unit.getGroup())) ++count;
        return count;
    }

    void showInventory() const {
        cout << "\n--- Blood Inventory ---\n";
        cout << left << setw(8) << "UnitID" << setw(10) << "Group"
             << setw(14) << "Expiry" << setw(12) << "Status" << '\n';
        for (const auto& unit : units) {
            string status = unit.isExpired() ? "EXPIRED" : (unit.isAllocated() ? "Allocated" : "Available");
            cout << left << setw(8) << unit.getId() << setw(10) << unit.getGroup()
                 << setw(14) << unit.getExpiryDate() << setw(12) << status << '\n';
        }
        if (units.empty()) cout << "No blood units recorded.\n";
    }

    void showRequests() const {
        cout << "\n--- Blood Requests ---\n";
        cout << left << setw(6) << "ID" << setw(20) << "Hospital" << setw(18)
             << "Patient" << setw(8) << "Group" << setw(7) << "Units"
             << setw(10) << "Urgency" << setw(12) << "Status" << '\n';
        for (const auto& request : requests) {
            const char* urgency = request.getUrgency() == 3 ? "Critical" :
                                  request.getUrgency() == 2 ? "Urgent" : "Routine";
            cout << left << setw(6) << request.getId() << setw(20) << request.getHospital().substr(0,18)
                 << setw(18) << request.getPatientName().substr(0,16) << setw(8) << request.getGroup()
                 << setw(7) << request.getUnits() << setw(10) << urgency
                 << setw(12) << (request.isFulfilled() ? "Fulfilled" : "Pending") << '\n';
        }
        if (requests.empty()) cout << "No requests recorded.\n";
    }

    void allocateRequests() {
        // Highest urgency first; ties go to earlier request ID (stable sort).
        vector<size_t> order;
        for (size_t i = 0; i < requests.size(); ++i)
            if (!requests[i].isFulfilled()) order.push_back(i);
        stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) {
            if (requests[a].getUrgency() != requests[b].getUrgency())
                return requests[a].getUrgency() > requests[b].getUrgency();
            return requests[a].getId() < requests[b].getId();
        });

        bool anyAllocation = false;
        for (size_t idx : order) {
            BloodRequest& req = requests[idx];
            int allocatedCount = 0;
            // Earliest-expiring compatible units first (FEFO).
            vector<size_t> candidates;
            for (size_t i = 0; i < units.size(); ++i) {
                if (!units[i].isAllocated() && !units[i].isExpired() &&
                    compatibleRedCells(req.getGroup(), units[i].getGroup()))
                    candidates.push_back(i);
            }
            stable_sort(candidates.begin(), candidates.end(), [&](size_t a, size_t b) {
                return units[a].getExpiryDate() < units[b].getExpiryDate();
            });

            for (size_t unitIndex : candidates) {
                if (allocatedCount >= req.getUnits()) break;
                units[unitIndex].allocate();
                ++allocatedCount;
                ostringstream entry;
                entry << todayDate() << " | Request #" << req.getId()
                      << " | Hospital: " << req.getHospital()
                      << " | Patient: " << req.getPatientName()
                      << " | Unit #" << units[unitIndex].getId()
                      << " (" << units[unitIndex].getGroup() << ")";
                history.push_back(entry.str());
                cout << "Allocated Unit #" << units[unitIndex].getId()
                     << " (" << units[unitIndex].getGroup() << ") to Request #"
                     << req.getId() << ".\n";
                anyAllocation = true;
            }
            if (allocatedCount == req.getUnits()) {
                req.markFulfilled();
                cout << "Request #" << req.getId() << " fulfilled.\n";
            } else if (allocatedCount > 0) {
                cout << "Request #" << req.getId() << " partially fulfilled ("
                     << allocatedCount << "/" << req.getUnits() << " units).\n";
            } else {
                cout << "Request #" << req.getId() << " could not be fulfilled; no compatible units available.\n";
            }
        }
        if (!anyAllocation) cout << "No allocations made.\n";
        save();
    }

    void showHistory() const {
        cout << "\n--- Allocation History ---\n";
        if (history.empty()) { cout << "No allocation history yet.\n"; return; }
        for (const auto& entry : history) cout << entry << '\n';
    }

    void save() const {
        ofstream out("blood_data.txt");
        if (!out) throw runtime_error("Could not write blood_data.txt");
        out << nextUnitId << '\n' << nextRequestId << '\n';
        out << units.size() << '\n';
        for (const auto& u : units)
            out << u.getId() << '|' << u.getGroup() << '|' << u.getExpiryDate() << '|' << u.isAllocated() << '\n';
        out << requests.size() << '\n';
        for (const auto& r : requests)
            out << r.getId() << '|' << r.getHospital() << '|' << r.getPatientName() << '|'
                << r.getGroup() << '|' << r.getUnits() << '|' << r.getUrgency() << '|'
                << r.getDate() << '|' << r.isFulfilled() << '\n';
        out << history.size() << '\n';
        for (const auto& h : history) out << h << '\n';
    }

    void load() {
        ifstream in("blood_data.txt");
        if (!in) return;
        try {
            in >> nextUnitId >> nextRequestId;
            in.ignore(numeric_limits<streamsize>::max(), '\n');
            size_t count = 0;
            string line;
            if (!(in >> count)) return;
            in.ignore(numeric_limits<streamsize>::max(), '\n');
            for (size_t i = 0; i < count; ++i) {
                getline(in, line);
                stringstream ss(line);
                string id, group, expiry, allocated;
                getline(ss,id,'|'); getline(ss,group,'|'); getline(ss,expiry,'|'); getline(ss,allocated,'|');
                units.emplace_back(stoi(id), group, expiry);
                if (allocated == "1") units.back().allocate();
            }
            if (!(in >> count)) return;
            in.ignore(numeric_limits<streamsize>::max(), '\n');
            for (size_t i = 0; i < count; ++i) {
                getline(in, line);
                stringstream ss(line);
                string id, hospital, patient, group, unitsCount, urgency, date, fulfilled;
                getline(ss,id,'|'); getline(ss,hospital,'|'); getline(ss,patient,'|'); getline(ss,group,'|');
                getline(ss,unitsCount,'|'); getline(ss,urgency,'|'); getline(ss,date,'|'); getline(ss,fulfilled,'|');
                BloodRequest r(stoi(id), hospital, patient, group, stoi(unitsCount), stoi(urgency));
                // Date is generated at load time in this teaching prototype.
                if (fulfilled == "1") r.markFulfilled();
                requests.push_back(r);
            }
            if (!(in >> count)) return;
            in.ignore(numeric_limits<streamsize>::max(), '\n');
            for (size_t i = 0; i < count; ++i) {
                getline(in, line);
                history.push_back(line);
            }
        } catch (...) {
            // If file data is damaged, start with whatever valid records were read.
            cerr << "Warning: some saved data could not be loaded.\n";
        }
    }
};

class DonorMatcher {
    vector<Donor> donors;
    int nextDonorId = 1;
public:
    DonorMatcher() { load(); }

    void addDonor(const string& name, const string& group, const string& phone) {
        donors.emplace_back(nextDonorId++, name, group, phone, true);
        save();
    }

    void showDonors() const {
        cout << "\n--- Donor List ---\n";
        cout << left << setw(6) << "ID" << setw(20) << "Name" << setw(10)
             << "Group" << setw(18) << "Phone" << setw(12) << "Status" << '\n';
        for (const auto& d : donors)
            cout << left << setw(6) << d.getId() << setw(20) << d.getName().substr(0,18)
                 << setw(10) << d.getBloodGroup() << setw(18) << d.getPhone().substr(0,16)
                 << setw(12) << (d.isAvailable() ? "Available" : "Unavailable") << '\n';
        if (donors.empty()) cout << "No donors registered.\n";
    }

    void findCompatibleDonors(const string& recipientGroup) const {
        cout << "\n--- Compatible Donors (Red-cell rule demo) ---\n";
        bool found = false;
        for (const auto& d : donors) {
            if (d.isAvailable() && compatibleRedCells(recipientGroup, d.getBloodGroup())) {
                cout << "Donor #" << d.getId() << " | " << d.getName()
                     << " | Group: " << d.getBloodGroup() << " | Phone: " << d.getPhone() << '\n';
                found = true;
            }
        }
        if (!found) cout << "No matching available donor found.\n";
        cout << "Note: this is only a software shortlist, not medical approval.\n";
    }

    void save() const {
        ofstream out("donors.txt");
        if (!out) throw runtime_error("Could not write donors.txt");
        out << nextDonorId << '\n' << donors.size() << '\n';
        for (const auto& d : donors)
            out << d.getId() << '|' << d.getName() << '|' << d.getBloodGroup() << '|'
                << d.getPhone() << '|' << d.isAvailable() << '\n';
    }

    void load() {
        ifstream in("donors.txt");
        if (!in) return;
        try {
            size_t count = 0;
            in >> nextDonorId >> count;
            in.ignore(numeric_limits<streamsize>::max(), '\n');
            for (size_t i = 0; i < count; ++i) {
                int id; string name, group, phone; bool available;
                char delimiter;
                in >> id >> delimiter;
                getline(in, name, '|'); getline(in, group, '|'); getline(in, phone, '|');
                in >> available;
                in.ignore(numeric_limits<streamsize>::max(), '\n');
                donors.emplace_back(id, name, group, phone, available);
            }
        } catch (...) {
            cerr << "Warning: donor data could not be loaded completely.\n";
        }
    }
};

void showMenu() {
    cout << "\n========== EMERGENCY BLOOD ALLOCATION & DONOR MATCHING ==========\n"
         << "1. Add blood unit to inventory\n"
         << "2. Register donor\n"
         << "3. Create hospital blood request\n"
         << "4. Show blood inventory\n"
         << "5. Show donor list\n"
         << "6. Find compatible donors for a blood group\n"
         << "7. Show hospital requests\n"
         << "8. Allocate blood to pending requests\n"
         << "9. Show allocation history\n"
         << "0. Exit\n"
         << "Select option: ";
}

int main() {
    try {
        AllocationManager allocation;
        DonorMatcher donorMatcher;

        while (true) {
            showMenu();

            string choiceLine;
            if (!getline(cin, choiceLine))
                break;

            stringstream ss(choiceLine);
            int choice;
            char extra;

            if (!(ss >> choice) || (ss >> extra)) {
                cout << "Invalid choice.\n";
                continue;
            }

            try {
                switch (choice) {
                    case 1: {
                        string group = readText("Blood group (O-, O+, A-, A+, B-, B+, AB-, AB+): ");
                        if (!isValidBloodGroup(group)) {
                            cout << "Invalid group.\n";
                            break;
                        }

                        string expiry = readText("Expiry date (YYYY-MM-DD): ");
                        allocation.addUnit(group, expiry);
                        cout << "Blood unit added.\n";
                        break;
                    }

                    case 2: {
                        string name = readText("Donor name: ");
                        string group = readText("Donor blood group: ");

                        if (!isValidBloodGroup(group)) {
                            cout << "Invalid group.\n";
                            break;
                        }

                        string phone = readText("Contact number: ");
                        donorMatcher.addDonor(name, group, phone);
                        cout << "Donor registered.\n";
                        break;
                    }

                    case 3: {
                        string hospital = readText("Hospital name: ");
                        string patient = readText("Patient name/ID: ");
                        string group = readText("Required blood group: ");

                        if (!isValidBloodGroup(group)) {
                            cout << "Invalid group.\n";
                            break;
                        }

                        int count = readInt("Units required: ", 1, 1000);
                        int urgency = readInt(
                            "Urgency (1=Routine, 2=Urgent, 3=Critical): ", 1, 3
                        );

                        allocation.addRequest(
                            hospital, patient, group, count, urgency
                        );

                        cout << "Request created.\n";
                        break;
                    }

                    case 4:
                        allocation.showInventory();
                        break;

                    case 5:
                        donorMatcher.showDonors();
                        break;

                    case 6: {
                        string group = readText("Recipient blood group: ");

                        if (!isValidBloodGroup(group)) {
                            cout << "Invalid group.\n";
                            break;
                        }

                        donorMatcher.findCompatibleDonors(group);
                        break;
                    }

                    case 7:
                        allocation.showRequests();
                        break;

                    case 8:
                        allocation.allocateRequests();
                        break;

                    case 9:
                        allocation.showHistory();
                        break;

                    case 0:
                        cout << "Exiting. Data is saved in text files.\n";
                        return 0;

                    default:
                        cout << "Choose a menu option from 0 to 9.\n";
                }
            }
            catch (const exception& e) {
                cout << "Error: " << e.what() << '\n';
            }
        }
    }
    catch (const exception& e) {
        cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
