#include "P2random.h"
#include<getopt.h>
#include <queue>
#include <limits>
#include<iostream>
#include <functional>
#include <vector>

using namespace std;

// ----------------------------------------------------------------------------
//                    Supplementary ADTs
// ----------------------------------------------------------------------------
enum class State {Initial, SeenOne, SeenBoth, MaybeBetter};

struct B {
    unsigned int priority;
    mutable unsigned int troop;
    unsigned int Source_index;
    int idf;
};

struct A{
    unsigned int priority;
    mutable unsigned int troop;
    unsigned int Source_index;
    int idf;
};

struct B_observe {
    unsigned int priority;
    unsigned int time;
};

struct A_observe {
    unsigned int priority;
    unsigned int time;
};

struct BPri { //higher priority--> higher priority
    bool operator()(const B S1, const B S2) const{
        //return S1.priority<S2.priority; 
        if(S1.priority==S2.priority)
        { 
            return S1.idf > S2.idf; }
        else{
            return S1.priority<S2.priority;
        }
    }
};

struct APri { //lower priority--> higher priority
    bool operator()(const A J1, A J2) const{
        //return J1.priority > J2.priority;
        if(J1.priority == J2.priority){
            return J1.idf > J2.idf;
        }
        else {
            return J1.priority > J2.priority;
        }
    }
};

class Median_helper {
 private:
  //Values greater than the median sorted so that smallest is on top
  std::priority_queue<int, std::vector<int>, std::greater<int> > second;
  //Values smaller than the median sorted so that greatest is on top
  std::priority_queue<int, std::vector<int>, std::less<int> > first;

 public:
  Median_helper(){
    //Seed the queues
    second.push((int) std::numeric_limits<int>::max());
    first.push ((int) std::numeric_limits<int>::min());
  }
  void push(int val){
    if(val>=second.top())
      second.push(val);
    else 
      first.push(val);
    if(second.size()-first.size()==2){ 
      first.push(second.top());        
      second.pop();                   
    } else if(first.size()-second.size()==2){ 
      second.push(first.top());               
      first.pop();                        
    }
  }

  int median() const {
    if(second.size()==first.size())               
      return(second.top()+first.top())/((int)2);  
    else if(second.size()>first.size())          
      return second.top();
    else                                        
      return first.top();
  }
};

class Agent {
    public:
        std::priority_queue<B, std::vector<B>, BPri> BQue;
        std::priority_queue<A, std::vector<A>, APri> AQue;

        //for the median mode
        //vector<unsigned int> information_lost_first_half, information_lost_second_half;
        Median_helper information_lost;

        //for the observer mode
        State stateRI = State::Initial;
        State statePI = State::Initial;
        B_observe best_RI_B, best_PI_B, maybe_RI_B, maybe_PI_B;
        A_observe best_RI_A, best_PI_A, maybe_RI_A, maybe_PI_A;
        unsigned int RI_gap = 0, PI_gap =0;
};

class Source {
    public:
        unsigned int A_total=0, B_total=0;
        unsigned int death=0;
};


// ----------------------------------------------------------------------------
//                    Information_sim Declarations
// ----------------------------------------------------------------------------
class Information_sim {
    public:
        void get_options(int argc, char** argv);
        void read_data();
        void write_results();
        void Source_mode();
        void observer_state_A(unsigned int pla_index, A_observe j);
        void observer_state_B(unsigned int pla_index, B_observe s);
        void observer_writer();
        void median_mode(unsigned int timestamp);
        void battle_judge(unsigned int pla_index);
        int num_bat=0;
    private:
        bool verbose=false, median=false, observer=false, source=false;
        vector<Agent> pla_vec;
        vector<Source> genr_vec;
        uint32_t num_ger, num_pla;
};


// ----------------------------------------------------------------------------
//                    Information_sim Implementation
// ----------------------------------------------------------------------------

void Information_sim::get_options(int argc, char * argv[]){
   int choice = 0, index=0;
   struct option long_options[] = {{ "verbose", no_argument, nullptr, 'v' },
                              { "median", no_argument, nullptr, 'm' }, 
                              {"source-eval", no_argument, nullptr, 'g'},
                              {"observer", no_argument, nullptr, 'w'},
                             { nullptr, 0, nullptr, '\0' }, 
                             };

   while ((choice = getopt_long(argc, argv, "vmgw", 
      long_options, &index)) != -1){
      switch(choice){
         case 'v':
            verbose = true;
            break;
         case 'm':
            median = true;
            break;
         case 'g':
            source = true;
            break;
         case 'w':
            observer = true;
            break;
      }
   }
}

void Information_sim::read_data(){
    bool DL=false; //if true, then PR mode
    stringstream ss;
    char var1, var2, var3, var4, var5, var6, var7, var8;
    unsigned int time, gen, agent, priority, dep;
    int gen_check, pla_check, priority_check, dep_check;

    // basic seting
    string temp;
    std::getline(cin,temp); std::getline(cin,temp);
    if(temp=="MODE: DL"){ DL=true;}
    std::getline(cin,temp); num_ger = (unsigned int)stoi (temp.substr(14,16));
    std::getline(cin,temp); num_pla = (unsigned int)stoi (temp.substr(13,15));
    pla_vec.resize(num_pla);
    if(source) { genr_vec.resize(num_ger);}


    // read in data based on different mode
    if(DL==false) {
        std::getline(cin,temp);
        uint32_t seed = (unsigned int)stoi(temp.substr(13,temp.length()-1));
        std::getline(cin,temp);
        uint32_t num_deploys = (unsigned int)stoi(temp.substr(17,temp.length()-1));
        std::getline(cin,temp);
        uint32_t rate = (unsigned int)stoi(temp.substr(14,temp.length()-1));
        P2random::PR_init(ss, seed,num_ger, num_pla,num_deploys,rate);
    } 

    istream &inputStream = DL == false ? ss : cin;
    unsigned int time_count=0;
    int index_input = 0;
    //int battle_acount=0;

    while(inputStream >> time>> var1 >> var2 >> var3 >> 
            var4 >> var5 >> gen_check >> var6 >>pla_check>>var7>>priority_check>>var8>>dep_check){
        
        
        agent = (unsigned int) pla_check;
        gen = (unsigned int) gen_check;
        priority = (unsigned int) priority_check;
        dep = (unsigned int) dep_check;

        // Error checking
        if(priority_check<=0 || dep_check<=0){
            cerr<<"Invalid priority-sensitivity or num_inform!";
            exit(1);
        }
        if(time < time_count){
            cerr<<"decreasing timestamps!!";
            exit(1);
        }
        if(gen_check<0 || gen>= num_ger) {
            cerr<<"Invalid Source ID!!";
            exit(1);
        }
        if(pla_check<0 || agent>=num_pla) {
            cerr<<"Invalid Agent ID!!";
            exit(1);
        }

        // Read
        if(var1=='A') 
        {   A j = {priority, dep, gen, index_input};
             pla_vec[agent].AQue.push(j); 
            if(source) {  genr_vec[gen].A_total+=dep; }
            if(observer) { 
                A_observe j ={priority, time};
                observer_state_A(agent, j);

            }
        } else {
            B s = {priority, dep, gen, index_input};
            pla_vec[agent].BQue.push(s); 
            if(source) { genr_vec[gen].B_total+=dep; }
            if(observer) { 
                B_observe s = {priority, time};
                observer_state_B(agent, s);
            }
        }
        
        index_input +=1;


        if(median) {
            if(time != time_count && num_bat!=0)
            {
                median_mode(time_count);
                time_count=time;
                //battle_acount=num_bat;
            }
        }

        if(time != time_count){
                time_count=time;
        }

        battle_judge(agent);
    
    }

    // Take the last timestamp data
    if(num_bat!=0 && median){
        median_mode(time_count); }

    std::cout <<"---End of Simulation---\n";
    std::cout <<"Interference: "<<num_bat<<"\n";
    
    size_t A_supporter =0;
    std::cout <<"---Agent Analysis---\n";
    size_t count=0;
    for(Agent &i:pla_vec){
        size_t A_sum=0, B_sum=0;
        for(size_t m=0; m<i.BQue.size(); m++){
            B_sum+=i.BQue.top().troop;
            i.BQue.pop();
        }
        for(size_t n=0; n<i.AQue.size(); n++){
            A_sum+=i.AQue.top().troop;
            i.AQue.pop();
        }
        string decision = A_sum>B_sum ?"A":"B";
        A_supporter = A_sum>B_sum ? A_supporter+1: A_supporter;
        std::cout <<"agent "<<count<<": support "<<decision<<"\n";
        count+=1;
    }
    std::cout <<"A_supporter% :"<<A_supporter/num_pla<<"\n";

    // Special modes
    if(source) { Source_mode(); }
    if(observer) { observer_writer(); }
}

void Information_sim::battle_judge(unsigned int pla_index){
    while(pla_vec[pla_index].BQue.size() != 0 && pla_vec[pla_index].AQue.size() != 0
         && pla_vec[pla_index].BQue.top().priority>=pla_vec[pla_index].AQue.top().priority){
        num_bat+=1;
        //cout << "(SJ) Size of PQ: "<<pla_vec[pla_index].BQue.size() <<" "<<pla_vec[pla_index].AQue.size()<<endl;
        const B & s = pla_vec[pla_index].BQue.top();
        const A & j = pla_vec[pla_index].AQue.top();
        unsigned int Stro = s.troop, Jtro = j.troop;
        //cout << "PQ top troop: "<<Stro<<" "<<Jtro<<endl;
        //cout << "PQ top priority: "<<s.priority<<" "<<j.priority<<endl;
        unsigned int lose = std::min(Stro, Jtro);
        s.troop = s.troop-lose;
        j.troop =j.troop -lose;

        //report this battle --verbose
        if(verbose) {
            std::cout << "Information from the source "<<s.Source_index<<" has Retroactive Interference with the information from the source "<<
                j.Source_index<<" on agent "<<pla_index<<". "<<lose*2<<
                " information intensity disappears.\n";
        }

        if(median){
            int total_lose = (int) lose*2;
            //cout<<total_lose;
            pla_vec[pla_index].information_lost.push(total_lose);
        }

        if(source){
            genr_vec[s.Source_index].death+=lose;
            genr_vec[j.Source_index].death+=lose;
        }

        if(s.troop==0) { 
            //cout <<"the previous B is poped out"<<endl;
            pla_vec[pla_index].BQue.pop();
            //cout <<"next top's data (priority,dep): "<<pla_vec[pla_index].BQue.top().priority<<" "<<pla_vec[pla_index].BQue.top().troop<<endl;
        }
        if(j.troop==0) {
            //cout <<"the previous A is poped out"<<endl;
            pla_vec[pla_index].AQue.pop();
            //cout <<"next top's data (priority,dep): "<<pla_vec[pla_index].AQue.top().priority<<" "<<pla_vec[pla_index].AQue.top().troop<<endl;
        }
        


    }
}

void Information_sim::median_mode(unsigned int timestamp){
    for(unsigned int i=0; i<pla_vec.size();i++){
        if(pla_vec[i].information_lost.median()!=0 && pla_vec[i].information_lost.median()!=2147483647) {
        std::cout << "Median information intensity lost on agent "<<i<<" at time "<<timestamp
        <<" is "<<pla_vec[i].information_lost.median()<< ".\n";
        }
    }
}

void Information_sim::Source_mode(){
    std::cout<<"---Source Evaluation---\n";
    for(unsigned int i=0;i<genr_vec.size();i++){
        std::cout << "Source "<<i<<" delievered "<<genr_vec[i].A_total
        <<" units of A type information "<<genr_vec[i].B_total<< " units of B type information, and "<<
        genr_vec[i].A_total+genr_vec[i].B_total-genr_vec[i].death<<"/"<<genr_vec[i].A_total+genr_vec[i].B_total<<" preserved.\n";
    }
}

void Information_sim::observer_state_A(unsigned int pla_index, A_observe J){
    //for RI
    if(pla_vec[pla_index].stateRI==State::Initial){ 
        pla_vec[pla_index].stateRI = State::SeenOne; 
        pla_vec[pla_index].best_RI_A = J;
    }
    if(pla_vec[pla_index].stateRI==State::SeenOne){
        if(pla_vec[pla_index].best_RI_A.priority>J.priority){
            pla_vec[pla_index].best_RI_A = J;
        }
    }
    if(pla_vec[pla_index].stateRI==State::SeenBoth){
        if(pla_vec[pla_index].best_RI_A.priority>J.priority){
            pla_vec[pla_index].stateRI = State::MaybeBetter; 
            pla_vec[pla_index].maybe_RI_A = J;
        }
    }
    if(pla_vec[pla_index].stateRI==State::MaybeBetter){
        if(pla_vec[pla_index].maybe_RI_A.priority>J.priority){
            pla_vec[pla_index].maybe_RI_A = J;
        }
    }

    //for PI
    if(pla_vec[pla_index].statePI==State::SeenOne){
        if(J.priority <= pla_vec[pla_index].best_PI_B.priority)
        {
            pla_vec[pla_index].statePI=State::SeenBoth;
            pla_vec[pla_index].best_PI_A = J;
            pla_vec[pla_index].PI_gap = pla_vec[pla_index].best_PI_B.priority-J.priority;
            /*
            cout << "Okay, so we've found one best PI! "<<" on Agent " 
                <<pla_index<< " with Sith at time "<<pla_vec[pla_index].best_PI_sith.time <<
                " and A at time "<< pla_vec[pla_index].best_PI_A.time 
                <<" with a priority difference of "<<pla_vec[pla_index].PI_gap<<".\n";
            */
        }
    }
    if(pla_vec[pla_index].statePI==State::SeenBoth){
        if(pla_vec[pla_index].best_PI_A.priority>J.priority){
            pla_vec[pla_index].best_PI_A = J;
            pla_vec[pla_index].PI_gap = pla_vec[pla_index].best_PI_B.priority-J.priority;
            /*
            cout <<"Ohh we found a better victim A! time: "<<pla_vec[pla_index].best_PI_A.time<<
            " priority: "<<pla_vec[pla_index].best_PI_A.priority<<"\n";
            */
        }
    }
    if(pla_vec[pla_index].statePI==State::MaybeBetter){
        if(J.priority <=pla_vec[pla_index].maybe_PI_B.priority) {
        if(pla_vec[pla_index].maybe_PI_B.priority-J.priority > pla_vec[pla_index].PI_gap){
            pla_vec[pla_index].best_PI_A = J;
            pla_vec[pla_index].best_PI_B = pla_vec[pla_index].maybe_PI_B;
            pla_vec[pla_index].PI_gap =pla_vec[pla_index].best_PI_B.priority-J.priority;
            pla_vec[pla_index].statePI=State::SeenBoth;
        }
        }
    }
}

void Information_sim::observer_state_B(unsigned int pla_index, B_observe S){
    //for RI
    if(pla_vec[pla_index].stateRI==State::SeenOne){
        if(S.priority >= pla_vec[pla_index].best_RI_A.priority)
        {
            pla_vec[pla_index].stateRI=State::SeenBoth;
            pla_vec[pla_index].best_RI_B = S;
            pla_vec[pla_index].RI_gap = S.priority-pla_vec[pla_index].best_RI_A.priority;
        }
    }
    if(pla_vec[pla_index].stateRI==State::SeenBoth){
        if(pla_vec[pla_index].best_RI_B.priority<S.priority){
            pla_vec[pla_index].best_RI_B = S;
            pla_vec[pla_index].RI_gap = S.priority-pla_vec[pla_index].best_RI_A.priority;
        }
    }
    if(pla_vec[pla_index].stateRI==State::MaybeBetter){
        if(S.priority >= pla_vec[pla_index].maybe_RI_A.priority) {
            if(S.priority-pla_vec[pla_index].maybe_RI_A.priority > pla_vec[pla_index].RI_gap){
                pla_vec[pla_index].best_RI_B = S;
                pla_vec[pla_index].best_RI_A = pla_vec[pla_index].maybe_RI_A;
                pla_vec[pla_index].RI_gap =S.priority-pla_vec[pla_index].best_RI_A.priority;
                pla_vec[pla_index].stateRI=State::SeenBoth;
        }}
    }
    //for PI
    if(pla_vec[pla_index].statePI==State::Initial) { 
        pla_vec[pla_index].statePI = State::SeenOne; 
        pla_vec[pla_index].best_PI_B = S;
    }
    if(pla_vec[pla_index].statePI==State::SeenOne){
        if(pla_vec[pla_index].best_PI_B.priority<S.priority){
            pla_vec[pla_index].best_PI_B = S;
        }
    }
    if(pla_vec[pla_index].statePI==State::SeenBoth){
        if(pla_vec[pla_index].best_PI_B.priority<S.priority){
            pla_vec[pla_index].statePI = State::MaybeBetter;
            pla_vec[pla_index].maybe_PI_B = S;
        }
    }
    if(pla_vec[pla_index].statePI==State::MaybeBetter){
        if(pla_vec[pla_index].maybe_PI_B.priority<S.priority){
            pla_vec[pla_index].maybe_PI_B = S;
        }
    }
}

void Information_sim::observer_writer(){
    std::cout<<"---Observer---\n";
    for(unsigned int i=0; i<pla_vec.size();i++){
        //PI
        if(pla_vec[i].statePI==State::Initial || pla_vec[i].statePI==State::SeenOne){
            std::cout<<"An observer would not see an optimal Proactive Interference on agent "<<i<<".\n";
        } else{
            std::cout<<"An observer would see an optimal Proactive Interference" 
                " on agent " <<i<< " with B at time " << pla_vec[i].best_PI_B.time 
                <<" and A at time "<<pla_vec[i].best_PI_A.time <<
                " with a priority difference of "<<pla_vec[i].PI_gap<<".\n";
        }
        //RI
        if(pla_vec[i].stateRI==State::Initial || pla_vec[i].stateRI==State::SeenOne){
            std::cout<<"An observer would not see an optimal Retroactive Interference on agent "<<i<<".\n";
        } else {
            std::cout<<"An observer would see an optimal Retroactive Interference" 
                " on agent " <<i<< " with A at time " << pla_vec[i].best_RI_A.time 
                <<" and B at time "<<pla_vec[i].best_RI_B.time <<
                " with a priority difference of "<<pla_vec[i].RI_gap<<".\n";
        }
    }
}

// ----------------------------------------------------------------------------
//                               Driver
// ----------------------------------------------------------------------------
int main(int argc, char** argv) {
    ios_base::sync_with_stdio(false);
    std::cout << "Simulating...\n";
    Information_sim simulator;
    simulator.get_options(argc, argv);
    simulator.read_data();
}