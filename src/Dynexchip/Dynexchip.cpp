// Copyright (c) 2021-2022, The Dynex Project
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <future>
#include <numeric>
#include <sstream>
#include <iostream>
#include <thread>
#include <cassert>
#include <atomic>
#include <random>

#include <string.h>
#include <exception>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>


#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

//using std::cout;
//using std::endl;
//using std::vector;
//using std::string;
//using std::pair;

using namespace boost::asio;
namespace ptime = boost::posix_time;
//using namespace std;
namespace pt = boost::property_tree;

std::vector<std::string> splitString(const std::string& str, char delim);

// ODE integration types:
typedef double value_type;
typedef std::vector< value_type > state_type;

// debugging output:
#define dynex_debugger false

// compatible job types:
#define JOB_TYPE_SAT 	1
#define JOB_TYPE_TUNING 2

// protocol definitions:
#define PROT_JOB_POSTED 0x0a
#define PROT_JOB_CANCELLED 0x1a
#define PROT_JOB_ACCEPTED 0x0b
#define PROT_JOB_CREDENTIALS 0x1b
#define PROT_JOB_CONFIGURED 0x0c
#define PROT_JOB_RUNNING 0x0d
#define PROT_JOB_UPDATED 0x0e
#define PROT_JOB_FINISHED 0x0f

// state machine definitions:
#define DYNEX_STATE_OFF -1
#define DYNEX_STATE_IDLE 0
#define DYNEX_STATE_ACCEPTED 1
#define DYNEX_STATE_CREDENTIALS 2
#define DYNEX_STATE_CONFIGURED 3
#define DYNEX_STATE_RUNNING 4
#define DYNEX_STATE_FINISHED 5

// dynex chip definitions:
#define         MAX_LITS_SYSTEM 25
#define         PARAM_COUNT     9

// FTP server for jobs:
#define FTP_ADDRESS "ftp.dynexcoin.org"
#define FTP_PORT "21"

// Dynex colors
#ifdef WIN32
#define TEXT_DEFAULT  ""
#define TEXT_YELLOW   ""
#define TEXT_GREEN    ""
#define TEXT_RED      ""
#define TEXT_BLUE     ""
#define TEXT_CYAN     ""
#define TEXT_WHITE    ""
#define TEXT_SILVER   ""
#else
#define TEXT_DEFAULT  "\033[0m"
#define TEXT_YELLOW   "\033[1;33m"
#define TEXT_GREEN    "\033[1;32m"
#define TEXT_RED      "\033[1;31m"
#define TEXT_BLUE     "\033[1;34m"
#define TEXT_CYAN     "\033[1;36m"
#define TEXT_WHITE    "\033[1;37m"
#define TEXT_SILVER   "\033[1;315m"
#endif 


#include "CryptoNoteCore/Currency.h" // CryptoNote::AccountPublicAddress

//---------------------------------------------------------------------------------------------------------------------------
// oberver & protocol handler
//---------------------------------------------------------------------------------------------------------------------------

class dynex_chip_thread_obj {
	
	std::promise<void> exitSignal;
    std::future<void> futureObj;

	public:
		// fee address:
		std::string addr_string;
		// my fee:
		uint64_t my_minute_rate;
		// job vars:
		int job_user_id;
    	int job_id;
    	uint64_t job_maxrate;
    	int job_type;
    	int job_dynexchips;
    	int job_available_slot;
    	uint64_t job_max_walltime;
    	std::string job_created;
    	std::string job_ftp_address;
    	std::string job_ftp_user;
    	std::string job_ftp_pw;
    	std::string job_param_01;
    	std::string job_param_02;
    	std::string job_param_03;
    	std::string job_param_04;
    	std::string job_param_05;
    	std::string job_param_06;
    	std::string job_max_simtime;
    	std::string job_max_steps;
    	std::string job_max_xl;
    	std::string job_alpha_heuristics;
    	std::string job_params_to_tune;
    	std::string job_tuning_mode;
    	std::string job_switchfraction;
    	std::string job_warmstart_file;
    	std::string job_partable_file;
    	std::string job_seed;
    	std::string job_max_tune_iterations;
    	std::string job_input_file;
    	// input file vars:
    	int n;
    	int m;
    	int * cls; 
    	int * clauseSizes; 
    	int * numOccurrenceT;
    	int maxNumOccurences = 0;
    	int * occurrence;
    	int * occurenceCounter;

    	// engine vars:
    	int global_best_thread;
    	double dmm_alpha;
    	double dmm_beta;
    	double dmm_gamma;
    	double dmm_delta;
    	double dmm_epsilon;
    	double dmm_zeta;
    	double timeout;
    	double init_dt = 1e-8;
    	int seed;
    	int xl_max;
    	int maxsteps = INT_MAX;
    	int tune = 0; //TBC
    	int heuristics = 0; //TBC
    	int digits = 15;
    	volatile int running_threads = 0;
    	char LOC_FILE[1024];
    	char SOLUTION_FILE[1024];
    	char PROOF_OF_WORK_FILE[1024];
    	double *v_best;
    	int *loc_thread;
    	int *unit_clause_vars;
    	double *energy_thread;
    	double *energy_thread_min;
    	int *global_thread;
    	int *global_all_runs_thread;
    	double *time_thread;
    	double *time_thread_actual;
    	double *walltime_thread;
    	double *t_init;
    	double *t_begin_thread;
    	double *t_end_thread;
    	int *stepcounter;
    	double *initial_assignments;
    	double *thread_params;
    	double *partable;
    	double *defaults;
    	bool unit_clauses=false;
    	struct node {
		    int id;                 //chip-id
		    int *model;             //current assignment
		    int *temporal;          //temp assignment for oracle (not used in production)
		    int *optimal;           //best assignment and solution afterwards
		};
		int solved;
        int global;    
        double global_energy;
        

	    void operator()(int chip_id, int _thread_count, uint64_t dynex_minute_rate, std::string _addr_string, std::atomic<bool>& dynex_quit_flag)
	    {
	    	bool search_for_work = true;
	    	addr_string = _addr_string;
	    	my_minute_rate = dynex_minute_rate;

	    	while( !dynex_quit_flag ) {
	    		
	    		// if we don't have work, search for it:
	    		if (search_for_work) {
	    				std::cout << log_time() << TEXT_CYAN << "[DYNEX CHIP "<<chip_id<<"] SEARCHING WORK..." << TEXT_DEFAULT << std::endl;
		    			bool workfound = dynex_get_work(chip_id, _thread_count, dynex_minute_rate);
						// WORK FOUND?
						if (workfound) {
							search_for_work = false;
							std::cout << log_time() << TEXT_CYAN << "[DYNEX CHIP "<<chip_id<<"] FOUND MATCHING WORK [" << job_user_id << "_" << job_id << ", BUDGET=" << job_max_steps << " STEPS, JOB_TYPE=" << job_type << ", RATE=" << dynex_minute_rate << " nanoDNX/Chip/ksteps]" << TEXT_DEFAULT << std::endl;
							// START WORK:
							bool workfinished = dynex_work(chip_id, _thread_count, dynex_minute_rate , dynex_quit_flag);
							if (workfinished) {
								std::cout << log_time() << TEXT_CYAN << "[DYNEX CHIP "<<chip_id<<"] WORK SUCCESSFULLY FINISHED." << TEXT_DEFAULT << std::endl;
								search_for_work = true;
							} else {
								std::cout << log_time() << TEXT_CYAN << "[DYNEX CHIP "<<chip_id<<"] WORK CANCELLED." << TEXT_DEFAULT << std::endl;
								search_for_work = true;
							}
						} else {
							std::cout << log_time() << TEXT_CYAN << "[DYNEX CHIP "<<chip_id<<"] NO WORK FOUND AT YOUR OFFERED RATE." << TEXT_DEFAULT << std::endl;
						}
	    		}
	    		
	    		std::this_thread::sleep_for(std::chrono::milliseconds(15000));
	    		//sleep(15); // wait time in seconds for querying new jobs
	    	}
	    	return;
	    }
	private:
		// helpers: --------------------------------------------------------------------------------------------------------------------
		std::string make_string(boost::asio::streambuf& streambuf)
		{
		  return {boost::asio::buffers_begin(streambuf.data()), 
		          boost::asio::buffers_end(streambuf.data())};
		}

		std::vector<std::string> splitString(const std::string& str, char delim)
		{
		  std::vector<std::string> tokens;
		  if (str == "") return tokens;
		  std::string currentToken;
		  std::stringstream ss(str);
		  while (std::getline(ss, currentToken, delim))
		  {
		    tokens.push_back(currentToken);
		  }
		  return tokens;
		}
		std::string log_time() {
			const boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
			const boost::posix_time::time_duration td = now.time_of_day();
			const long hours = td.hours();
			const long minutes = td.minutes();
			const long seconds = td.seconds();
			const long milliseconds = td.total_milliseconds() - ((hours * 3600 + minutes * 60 + seconds) * 1000);
			char buf[40];
			sprintf(buf, "%02ld:%02ld:%02ld.%06ld ",hours, minutes, seconds, milliseconds);
    		return buf;
		}

		// work: ------------------------------------------------------------------------------------------------------------------------
		// allocates memory, sets params and initial assignments, then starts the ode integration
		bool dynex_work(int chip_id, int _thread_count, uint64_t dynex_minute_rate, std::atomic<bool>& dynex_quit_flag) {
			//std::cout << log_time() << TEXT_CYAN << "[DYNEX CHIP "<<chip_id<<"] STARTING WORK " << job_user_id << "_" << job_id << "..." << TEXT_DEFAULT << std::endl;
			// first load input file:
			bool inputfile_downloaded = download_input_file(chip_id);
			if (!inputfile_downloaded) {
				return false;
			}
			/// initialize arrays:
		    v_best = (double *) calloc((size_t) (n+m*2), sizeof(double));
		    loc_thread = (int *) calloc((size_t) 1, sizeof(int));
		    unit_clause_vars = (int *) calloc((size_t) n+1, sizeof(int));
		    energy_thread = (double *) calloc((size_t) 1, sizeof(double));
		    energy_thread_min = (double *) calloc((size_t) 1, sizeof(double));
		    global_thread = (int *) calloc((size_t) 1, sizeof(int));
		    global_all_runs_thread = (int *) calloc((size_t) 1, sizeof(int));
		    time_thread = (double *) calloc((size_t) 1, sizeof(double));
		    time_thread_actual = (double *) calloc((size_t) 1, sizeof(double));
		    walltime_thread = (double *) calloc((size_t) 1, sizeof(double));
		    t_init = (double *) calloc((size_t) 1, sizeof(double));
		    t_begin_thread = (double *) calloc((size_t) 1, sizeof(double));
		    t_end_thread = (double *) calloc((size_t) 1, sizeof(double));
		    stepcounter = (int *) calloc((size_t) 1, sizeof(int));
		    initial_assignments = (double *) calloc((size_t) (n+m*2), sizeof(double));
		    thread_params = (double *) calloc((size_t) PARAM_COUNT, sizeof(double));
		    partable = (double *) calloc((size_t) 4, sizeof(double));
		    defaults = (double *) calloc((size_t) 128, sizeof(double));
			/// detect (and assign) unit clauses (clauses with one literal): ----------------------------
		    int cnt_unit_clauses = 0;
		    for (int i=0; i<m; i++) {
		        if (clauseSizes[i]==1) {
		            cnt_unit_clauses++;
		            unit_clauses = true;
		            int lit = cls[i*MAX_LITS_SYSTEM];
		            if (lit>0) unit_clause_vars[abs(lit)] = 1;
		            if (lit<0) unit_clause_vars[abs(lit)] = -1;
		        }
		    }
		    if (dynex_debugger) printf("c %d UNIT CLAUSES DETECTED.\n",cnt_unit_clauses);
		    /// set _all_runs vars: ---------------------------------------------------------------------
		    global_all_runs_thread[0] = m;
		    /// set t_init: -----------------------------------------------------------------------------
		    t_init[0] = 0.0;
		    /// load assignment: ------------------------------------------------------------------------
		    // TBD
		    /// load partable: --------------------------------------------------------------------------
		    // TBD
		    /// OUPUT SETTINGS: -------------------------------------------------------------------------
		    dmm_alpha 	= std::atof(job_param_01.c_str());
    		dmm_beta 	= std::atof(job_param_02.c_str());
    		dmm_gamma 	= std::atof(job_param_03.c_str());
    		dmm_delta 	= std::atof(job_param_04.c_str());
    		dmm_epsilon = std::atof(job_param_05.c_str());
    		dmm_zeta 	= std::atof(job_param_06.c_str());
    		timeout 	= std::atof(job_max_simtime.c_str());
    		seed 		= std::atoi(job_seed.c_str()); //disregarded currently
    		xl_max 		= std::atoi(job_max_xl.c_str());
    		maxsteps    = std::atoi(job_max_steps.c_str());
		    
		    if (dynex_debugger) {
		        printf(TEXT_CYAN);
		        printf("c [%d] SETTINGS:\n",chip_id);
		        printf("c [%d] MAX STEPS       : ",chip_id); std::cout << maxsteps << std::endl;
		        printf("c [%d] TIMEOUT         : ",chip_id); std::cout << timeout << std::endl;
		        printf("c [%d] INITAL dt       : ",chip_id); std::cout << init_dt << std::endl;
		        printf("c [%d] HEURISTICS      : %d\n",chip_id,heuristics);
		        printf("c [%d] TUNE CIRCUIT    : %d\n",chip_id,tune);
		        std::cout << std::setprecision(digits) << std::fixed;
		        printf("c [%d] ALPHA           : ",chip_id); std::cout << dmm_alpha << std::endl; // %.17f\n",dmm_alpha);
		        printf("c [%d] BETA            : ",chip_id); std::cout << dmm_beta << std::endl;
		        printf("c [%d] GAMMA           : ",chip_id); std::cout << dmm_gamma << std::endl;
		        printf("c [%d] DELTA           : ",chip_id); std::cout << dmm_delta << std::endl;
		        printf("c [%d] EPSILON         : ",chip_id); std::cout << dmm_epsilon << std::endl;
		        printf("c [%d] ZETA            : ",chip_id); std::cout << dmm_zeta << std::endl;
		        printf("c [%d] SEED            : %d\n",chip_id,seed);
		        printf("c [%d] XL_MAX          : %.d\n",chip_id,xl_max);

		        printf(TEXT_DEFAULT);
		    }
		    /// init thread specific ODE parameters --------------------------------------------------------
		    /* RNG */
		    std::random_device rd;
			std::seed_seq sd{rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd()};
		    std::mt19937 generator(sd); 
		    std::uniform_real_distribution<double>  rand_v_double(-1.0, 1.0); // voltage continous between -1 and +1 
		    std::uniform_int_distribution<int>      rand_v(-1, 1); // voltage -1, 0 or +1
		    std::uniform_real_distribution<double>  rand_Xs(0.0, 1.0);
		    std::uniform_real_distribution<double>  rand_Xl(1.0, 100.0); 
		    
	        thread_params[0]    = dmm_alpha;
	        thread_params[1]    = dmm_beta;
	        thread_params[2]    = dmm_gamma;
	        thread_params[3]    = dmm_delta;
	        thread_params[4]    = dmm_epsilon;
	        thread_params[5]    = dmm_zeta;
	        thread_params[6]    = 0.0;
	        thread_params[7]    = 0.0;
	        thread_params[8]    = init_dt;  
		    //initial assignments:
            for (int j=0; j<n; j++) initial_assignments[j] = rand_v(generator);
            for (int j=n; j<n+m; j++) initial_assignments[j] =  0.0; //double(0.0); //rand_Xs(generator); //; 
            for (int j=n+m; j<n+m*2; j++) initial_assignments[j] = 1.0; //double(1.0); //rand_Xl(generator); // //double(1.0); //
	        // override unit clauses for initial assignments:
	        if (unit_clauses) {
	            for (int j=0; j<n; j++) {
	                if (unit_clause_vars[j+1]!=0) initial_assignments[j] = unit_clause_vars[j+1];
	            }
	        }
		    
		    /// prepare LOC file -----------------------------------------------------------------------------
		    strcpy(LOC_FILE, job_input_file.c_str());
		    std::string adds = "."+std::to_string(chip_id)+".loc.txt";
	        char APPEND[128]; strcpy(APPEND, adds.c_str());
	        strcat(LOC_FILE,APPEND);
            FILE *floc = fopen(LOC_FILE, "w");
            fprintf(floc,"INSTANCE;STEPS;WALLTIME;ODE_TIME;LOC;ENERGY\n");
            fprintf(floc,"%d;%d,%.5f;%.5f;%d;%d\n",0,0,0.0,0.0,m,m);
            fclose(floc);

            /// prepare SOLUTION file ------------------------------------------------------------------------
		    strcpy(SOLUTION_FILE, job_input_file.c_str());
		    adds = "."+std::to_string(chip_id)+".solution.txt";
		    char APPENDS[128]; strcpy(APPENDS,adds.c_str());
		    strcat(SOLUTION_FILE,APPENDS);

		    /// prepare PROOF OF WORK file -------------------------------------------------------------------
		    strcpy(PROOF_OF_WORK_FILE, job_input_file.c_str());
		    adds = "."+addr_string+"."+std::to_string(chip_id)+".proofofwork.txt";
		    char APPENDSS[128]; strcpy(APPENDSS,adds.c_str());
		    strcat(PROOF_OF_WORK_FILE,APPENDSS);

            /// run solver -----------------------------------------------------------------------------------
            apply(chip_id, dynex_quit_flag);
            
			return true;
		}

		// ---------------------------------------------------------------------------------------------------------------
		// DXDT CALCULATION:
		// ---------------------------------------------------------------------------------------------------------------
		std::vector<double> dmm_generate_dxdt(int chip_id, std::vector<double> x , double t ) {
			
			//timers:
        	t_end_thread[0] = ptime::microsec_clock::local_time().time_of_day().total_milliseconds();
        	double time_spent = (double)(t_end_thread[0] - t_begin_thread[0])/1000;
        	time_thread_actual[0] = t;
        	energy_thread[0] = 0.0;
        	//vectors:
        	std::vector<double> dxdt(n+m*2); // dxdt vector
        	std::vector< std::vector <double >> dxdt_v(n); // vector for storing all voltages
        	/* main ODE routine */
	        if (solved!=1) {
	            int loc;
	            // Loop ---------------------------------------------------------------------------------------------------------------
	            loc = m;
	            // screen info variables:
	            double C_rem, R_rem, G_rem;
	            int cnt_xl_pos = 0;
	            int cnt_xs_pos = 0;
	            // loop through each clause:
	            for (int clause = 0; clause < m; clause++) {
	                // number of literals in this clause:
	                int ksat = clauseSizes[clause];
	                // Xl & Xs:
	                double Xs = x[clause+n];   if (Xs<0.0) Xs = double(0.0); if (Xs>1.0) Xs = double(1.0); //Xs bounds
	                double Xl = x[clause+n+m]; if (Xl<1.0) Xl = double(1.0); if (Xl>xl_max) Xl = double(xl_max); //Xl bounds
	                double C  = double(0.0);
	                double Ri, Rj, Rk, Gi, Gj, Gk;
	                // 3-sat:
	                if (ksat==3) {
	                    int Qi = (cls[clause*MAX_LITS_SYSTEM+0]>0)? 1:-1; // +1 if literal is >0, otherwise -1
	                    int Qj = (cls[clause*MAX_LITS_SYSTEM+1]>0)? 1:-1; // +1 if literal is >0, otherwise -1
	                    int Qk = (cls[clause*MAX_LITS_SYSTEM+2]>0)? 1:-1; // +1 if literal is >0, otherwise -1
	                    int liti = abs(cls[clause*MAX_LITS_SYSTEM+0]);
	                    int litj = abs(cls[clause*MAX_LITS_SYSTEM+1]);
	                    int litk = abs(cls[clause*MAX_LITS_SYSTEM+2]);
	                    double Vi = x[liti-1]; if (Vi<-1.0) Vi = -1.0; if (Vi>1.0) Vi = 1.0; //V bounds
	                    double Vj = x[litj-1]; if (Vj<-1.0) Vj = -1.0; if (Vj>1.0) Vj = 1.0; //V bounds
	                    double Vk = x[litk-1]; if (Vk<-1.0) Vk = -1.0; if (Vk>1.0) Vk = 1.0; //V bounds
	                    double i = double(1.0)-double(Qi)*Vi;
	                    double j = double(1.0)-double(Qj)*Vj;
	                    double k = double(1.0)-double(Qk)*Vk;
	                    C = double(fmin(i, fmin(j, k)));
	                    C = C / double(2.0);
	                    if (C<0.0) C=double(0.0);
	                    if (C>1.0) C=double(1.0);
	                    
	                    // equation Gn,m(vn,vj,vk)= 1/2 qn,mmin[(1−qj,mvj),(1−qk,mvk)] (5.x):
	                    Gi = double(Qi) * fmin(j,k) / double(2.0);
	                    Gj = double(Qj) * fmin(i,k) / double(2.0);
	                    Gk = double(Qk) * fmin(i,j) / double(2.0);
	                    
	                    // equation Rn,m (vn , vj , vk ) = 1/2(qn,m −vn), Cm(vn,vj,vk)= 1/2(1−qn,mvn), 0 otherwise (5.x):
	                    if (C == double(i/double(2.0)) ) {Ri = (double(Qi)-Vi)/2.0;} else {Ri = double(0.0);} //Qi*i/2.0*-1;} //= 0.0
	                    if (C == double(j/double(2.0)) ) {Rj = (double(Qj)-Vj)/2.0;} else {Rj = double(0.0);} //Qj*j/2.0*-1;} //= 0.0
	                    if (C == double(k/double(2.0)) ) {Rk = (double(Qk)-Vk)/2.0;} else {Rk = double(0.0);} //Qk*k/2.0*-1;} //= 0.0
	                    
	                    // equation Vn = SUM xl,mxs,mGn,m + (1 + ζxl,m)(1 − xs,m)Rn,m (5.x):
	                    double _Vi = Xl * Xs * Gi + (double(1.0) + dmm_zeta * Xl) * (double(1.0) - Xs) * Ri ;
	                    double _Vj = Xl * Xs * Gj + (double(1.0) + dmm_zeta * Xl) * (double(1.0) - Xs) * Rj ;
	                    double _Vk = Xl * Xs * Gk + (double(1.0) + dmm_zeta * Xl) * (double(1.0) - Xs) * Rk ;

	                    //sum of vectors method:
	                    if (_Vi!=0.0) dxdt_v[liti-1].push_back(_Vi);
	                    if (_Vj!=0.0) dxdt_v[litj-1].push_back(_Vj);
	                    if (_Vk!=0.0) dxdt_v[litk-1].push_back(_Vk);

	                    // do not change unit_clauses:
	                    if (unit_clauses) {
	                        if (unit_clause_vars[liti]!=0) dxdt[liti-1] = 0.0;
	                        if (unit_clause_vars[litj]!=0) dxdt[litj-1] = 0.0;
	                        if (unit_clause_vars[litk]!=0) dxdt[litk-1] = 0.0;
	                    }
	                }
	                // 2-sat:
	                if (ksat==2) {
	                    int Qi = (cls[clause*MAX_LITS_SYSTEM+0]>0)? 1:-1; // +1 if literal is >0, otherwise -1
	                    int Qj = (cls[clause*MAX_LITS_SYSTEM+1]>0)? 1:-1; // +1 if literal is >0, otherwise -1
	                    int liti = abs(cls[clause*MAX_LITS_SYSTEM+0]);
	                    int litj = abs(cls[clause*MAX_LITS_SYSTEM+1]);
	                    double Vi = x[liti-1]; if (Vi<-1.0) Vi = -1.0; if (Vi>1.0) Vi = 1.0; 
	                    double Vj = x[litj-1]; if (Vj<-1.0) Vj = -1.0; if (Vj>1.0) Vj = 1.0;
	                    double i = double(1.0)-double(Qi)*Vi;
	                    double j = double(1.0)-double(Qj)*Vj;
	                    C = double(fmin(i, j));
	                    C = C / double(2.0) ;
	                    if (C<0.0) C=double(0.0);
	                    if (C>1.0) C=double(1.0);
	                    //voltage:
	                    Gi = double(Qi) * j / double(2.0);
	                    Gj = double(Qj) * i / double(2.0);
	                    
	                    if (C == double(i/double(2.0)) ) {Ri = (double(Qi)-Vi)/2.0;} else {Ri = double(0.0);} //Qi*i/2.0*-1;} //= 0.0
	                    if (C == double(j/double(2.0)) ) {Rj = (double(Qj)-Vj)/2.0;} else {Rj = double(0.0);} //Qj*j/2.0*-1;} //= 0.0

	                    double _Vi = Xl * Xs * Gi + (double(1.0) + dmm_zeta * Xl) * (double(1.0) - Xs) * Ri;
	                    double _Vj = Xl * Xs * Gj + (double(1.0) + dmm_zeta * Xl) * (double(1.0) - Xs) * Rj;

	                    //sum of vectors method:
	                    if (_Vi!=0.0) dxdt_v[liti-1].push_back(_Vi);
	                    if (_Vj!=0.0) dxdt_v[litj-1].push_back(_Vj);
	                    
	                    // do not change unit_clauses:
	                    if (unit_clauses) {
	                        if (unit_clause_vars[liti]!=0) dxdt[liti-1] = 0.0;
	                        if (unit_clause_vars[litj]!=0) dxdt[litj-1] = 0.0;
	                    }
	                }
	                
	                // k-sat:
	                if (ksat!=1 && ksat!=2 && ksat!=3) {
	                    int lit[MAX_LITS_SYSTEM], Q[MAX_LITS_SYSTEM];
	                    double V[MAX_LITS_SYSTEM], _i[MAX_LITS_SYSTEM], R[MAX_LITS_SYSTEM], G[MAX_LITS_SYSTEM];

	                    double c_min=INT_MAX;
	                    for (int i=0; i<ksat; i++) {
	                        Q[i] = (cls[clause*MAX_LITS_SYSTEM+i]>0)? 1:-1; // +1 if literal is >0, otherwise -1
	                        lit[i] = abs(cls[clause*MAX_LITS_SYSTEM+i]);
	                        V[i] = x[lit[i]-1]; if (V[i]<-1.0) V[i]=-1.0; if (V[i]>1.0) V[i]=1.0; //boundary for v € [-1,1]:
	                        _i[i] = double(1.0)-double(Q[i])*V[i];
	                        // find min:
	                        if (_i[i]<c_min) c_min = _i[i]; 
	                    }
	                    C = c_min / double(2.0);
	                    if (C<0.0) printf("*\n");//C=0.0; // never triggered?
	                    if (C>1.0) printf("*\n");//C=1.0; // never triggered?
	                    
	                    for (int i=0; i<ksat; i++) {
	                        //find min of others:
	                        double g_min = INT_MAX;
	                        for (int ii=0; ii<ksat; ii++) {if (ii!=i && _i[ii]<g_min) g_min = _i[ii];}
	                        G[i] = double(Q[i]) * g_min / double(2.0);
	                        double comp = _i[i]/double(2.0);
	                        if (comp<0.0) printf("*\n");//comp = 0.0; // never triggered?
	                        if (comp>1.0) printf("*\n");//comp = 1.0; // never triggered?
	                        if (C != comp) {R[i] = double(0.0);} else {R[i] = (double(Q[i])-V[i]) / double(2.0);}
	                        double _V = Xl * Xs * G[i] + (double(1.0) + dmm_zeta * Xl) * (double(1.0) - Xs) * R[i];
	                        
	                        //sum of vectors method:
	                        if (_V!=0.0) dxdt_v[lit[i]-1].push_back(_V);
	                    
	                        // do not change unit_clauses:
	                        if (unit_clauses) {
	                            if (unit_clause_vars[lit[i]]!=0) dxdt[lit[i]-1] = 0.0;
	                        }
	                    }
	                }

	                //update energy:
	                energy_thread[0] += C;
	                //update loc:
	                if (C<0.5) loc--; //this clause is sat, reduce loc
	                
	                // Calculate new Xs:
	                dxdt[n+clause] = dmm_beta * (Xs + dmm_epsilon) * (C - dmm_gamma);
	                
	                // Calculate new Xl:
	                dxdt[n+m+clause] = dmm_alpha * (C - dmm_delta);
	                
	                // update info variables:
	                if (clause==0) {
	                    C_rem = C; 
	                    if (ksat<=3) {
	                        G_rem = Gi; R_rem = Ri;
	                    } else {
	                        G_rem = 0.0;
	                        R_rem = 0.0;
	                    }
	                    if (ksat==1) {
	                        G_rem = 0.0; 
	                        R_rem = 0.0;
	                    }
	                }
	                if (x[n+m+clause]>1.0) cnt_xl_pos++;
	                if (x[n+clause]>0.0) cnt_xs_pos++;
	            } //---clause calculation loop

	            // summation of voltages SUM dxdt_v[n] => dxdt[n]: ------------------------------------------------------
	            for (int i=0; i<n; i++) {
	                std::sort(dxdt_v[i].begin(), dxdt_v[i].end()); //summing with smallest first increases accuracy
	                dxdt[i] = accumulate(dxdt_v[i].begin(), dxdt_v[i].end(), (double) 0.0);
	            }

	            // update global_all_runs_thread & update v_best: --------------------------------------------------------
	            if (loc <= global_all_runs_thread[0]) {
	                global_all_runs_thread[0] = loc;
	                t_init[0] = t;
	                //update v_best array:
	                for (int i=0; i<n+m*2; i++) v_best[i] = x[i]; 
	            }
	            // update loc of thread: ---------------------------------------------------------------------------------
	            loc_thread[0] = loc;
	            time_thread_actual[0] = t;

	            //new lower lock (global)? or lower energy (global)? -----------------------------------------------------
	            if (loc<global || energy_thread[0]<global_energy) {
	                if (loc<global) {
	                    global = loc;
	                    global_best_thread = 0;
	                }
	                if (energy_thread[0]<global_energy) {
	                    global_energy = energy_thread[0];
	                    global_best_thread = 0;
	                }
	                if (energy_thread[0]<energy_thread_min[0]) {
	                    global_best_thread = 0;
	                }
	                if (loc<global_thread[0]) {
	                    global_best_thread = 0;
	                }

	                if (dynex_debugger) {
	                    std::cout << std::setprecision(2) << std::fixed << TEXT_DEFAULT
	                    << "\rc [" << chip_id << "] " << time_spent << "s "
	                    << "T=" << t
	                    << " GLOBAL=" << global
	                    << " (LOC=" << loc << ")" 
	                    << " (" << stepcounter[0] << ")"
	                    << " α=" << dmm_alpha
	                    << " β=" << dmm_beta
	                    << " C=" << C_rem
	                    << " (" << cls[0] << "=" << x[abs(cls[0])-1]
	                    << ", " << cls[1] << "=" << x[abs(cls[1])-1]
	                    << ", " << cls[2] << "=" << x[abs(cls[2])-1] << ")"
	                    << " Xs=" << x[n]
	                    << " Xl=" << x[n+m] 
	                    << " #Xs>0:" << cnt_xs_pos 
	                    << " #Xl>1:" << cnt_xl_pos 
	                    << " Σe=" << energy_thread[0] << " " << std::endl;
	                    fflush(stdout);
	                } 
	                // loc file; -----------------------------------------------------------------------------------------
                    FILE *floc = fopen(LOC_FILE, "a");
                    fprintf(floc,"%d;%d,%.5f;%.5f;%d;%.2f\n",chip_id, stepcounter[0], time_spent,t,global,global_energy);
                    fclose(floc);
	            }

	            // update energy of thread: ------------------------------------------------------------------------------
	            if (energy_thread[0] < energy_thread_min[0]) {
	                energy_thread_min[0] = energy_thread[0];
	            }

	            //new thread global? --------------------------------------------------------------------------------------
	            if (loc<global_thread[0]) {
	                // update global_thread, time_thread and walltime_thread:
	                global_thread[0] = loc;
	                time_thread[0] = t;
	                walltime_thread[0] = time_spent;
	            }
	            
	            //solved? -------------------------------------------------------------------------------------------------
	            if (loc==0) {
	            	if (dynex_debugger) {
		                printf(TEXT_YELLOW);
	                    printf("\nc [%d] T=",chip_id); std::cout << t << " SOLUTION FOUND" << std::endl; 
	                    for (int i=0; i<n; i++) {
	                        if (x[i]>0) printf("%d ",(i+1));
	                        if (x[i]<0) printf("%d ",(i+1)*-1);
	                    }
	                    fflush(stdout);
                    	printf("\nc [%d] VERIFYING...\n",chip_id); if (dynex_debugger) printf(TEXT_DEFAULT);
                	}
                    
                    bool sat = true; bool clausesat;
                    for (int i=0; i<m; i++) {
                        for (int j=0; j<clauseSizes[i]; j++) {
                            clausesat = false;
                            int lit = abs(cls[i*MAX_LITS_SYSTEM+j]);
                            if ( (x[lit-1]>0 && cls[i*MAX_LITS_SYSTEM+j]>0) || (x[lit-1]<0 && cls[i*MAX_LITS_SYSTEM+j]<0) ) {
                                clausesat = true;
                                break;
                            }
                        }
                        if (!clausesat) {
                            sat = false;
                            break;
                        }
                    }

                    if (dynex_debugger) printf(TEXT_YELLOW);
                    if (sat)  {
                        if (dynex_debugger) printf(TEXT_YELLOW); 
                        if (dynex_debugger) printf("c [%d] SAT (VERIFIED)\n",chip_id); 
                        solved = 1;
                        std::cout << log_time() << TEXT_CYAN << "[DYNEX CHIP " << chip_id << "] " << TEXT_YELLOW << "FOUND A SOLUTION." << TEXT_DEFAULT << std::endl;
                        //write solution to file:
                        FILE *fs = fopen(SOLUTION_FILE, "w");
                        for (int i=0; i<n; i++) {
                            if (x[i]>=0) fprintf(fs,"%d, ",i+1);
                            if (x[i]<0) fprintf(fs,"%d, ",(i+1)*-1);
                        }
                        fclose(fs);    
                    }
                    if (!sat) {
                    	if (dynex_debugger) printf(TEXT_RED); 
                    	if (dynex_debugger) printf("c [%d] UNSAT (VERIFIED)\n",chip_id);
                    }
                    if (dynex_debugger) printf(TEXT_DEFAULT);
	                

	                // update locfile:
                    FILE *floc = fopen(LOC_FILE, "a");
                    fprintf(floc,"%d;%d,%.5f;%.5f;%d;%.2f\n",chip_id, stepcounter[0], time_spent,t,global,global_energy);
                    fclose(floc);
            
	            } // ---output
	        }


		    return dxdt;
		}

		// ---------------------------------------------------------------------------------------------------------------
		// ODE INTEGRATION STEP:
		// ---------------------------------------------------------------------------------------------------------------
		std::vector<double> dmm_dostep(int chip_id, std::vector<double> _x, std::vector<double> dxdt, double h) {

		    //update V:
		    for (int i=0; i<n; i++) {
		        _x[i] = _x[i] + h * dxdt[i];
		        if (unit_clause_vars[i+1]!=0) _x[i] = unit_clause_vars[i+1]; //TODO: assign +1 or -1
		        if (_x[i]<-1.0) _x[i]=-1.0;   
		        if (_x[i]>1.0) _x[i]=1.0;
		    }
		    //update XS:
		    for (int i=n; i<n+m; i++) {
		        _x[i] = _x[i] + h * dxdt[i];
		        if (_x[i]<0.0) _x[i]=0.0;
		        if (_x[i]>1.0) _x[i]=1.0;
		    }
		    //update Xl:
		    for (int i=n+m; i<n+m*2; i++) {
		        _x[i] = _x[i] + h * dxdt[i];
		        if (_x[i]<1.0) _x[i]=1.0;  
		        if (_x[i]>xl_max) _x[i]=xl_max;
		    }
		    
		    // increase stepcounter for this thread:
		    stepcounter[0]++;

		    return _x;
		}

		// generate proof-of-work  ------------------------------------------------------------------------------------------------------
		bool dynex_proof_of_work(int chip_id) {
			// my wallet address:
			// my address is addr_string (std::string)

			//create proof-of-work file:
			FILE *fs = fopen(PROOF_OF_WORK_FILE, "w");
			std::time_t t = std::time(nullptr);
		    char mbstr[100];
		    std::strftime(mbstr, sizeof(mbstr), "%H:%M:%S %Y-%m-%d", std::localtime(&t));
		    fprintf(fs,"{{\"ADDRESS\":\"%s\"},{\"TIMESTAMP\":\"%s\"},{\"LOC\":%d},{\"STEPS\":%d},{\"RATE\":%llu}{\"DATA\":",addr_string.c_str(), mbstr, global,stepcounter[0], my_minute_rate );
			for (int i=0; i<n; i++) {
                if (v_best[i]>=0) fprintf(fs,"%d, ",i+1);
                if (v_best[i]<0) fprintf(fs,"%d, ",(i+1)*-1);
            }
            fprintf(fs,"}}");
            fclose(fs);

            //submit file:
            io_service ios;
			ip::tcp::resolver resolver(ios);
			ip::tcp::resolver::query query(job_ftp_address, FTP_PORT);
			ip::tcp::resolver::iterator it = resolver.resolve(query);
			ip::tcp::endpoint endpoint = *it;
			ip::tcp::socket client(ios);
			
			const int BUFLEN = 1024;
			std::vector<char> buf(BUFLEN);

			client.connect(endpoint);
			boost::system::error_code error;
			int len = client.receive(buffer(buf, BUFLEN), 0, error);
			if (dynex_debugger) {std::cout<<"RESPONSE: "; std::cout.write(buf.data(), len); std::cout << std::endl;}

			std::string request = "USER "+job_ftp_user+"\r\n";
			if (dynex_debugger) std::cout << request << std::endl;
			client.send(buffer(request, request.size()));
			len = client.receive(buffer(buf, BUFLEN), 0, error);
			if (dynex_debugger) {std::cout<<"RESPONSE: "; std::cout.write(buf.data(), len); std::cout << std::endl;}

			request = "PASS "+job_ftp_pw+"\r\n"; 
			if (dynex_debugger) std::cout << request << std::endl;
			client.send(buffer(request, request.size()));
			len = client.receive(buffer(buf, BUFLEN), 0, error);
			if (dynex_debugger) {std::cout<<"RESPONSE: "; std::cout.write(buf.data(), len); std::cout << std::endl;}
			if (len==33) {
				std::cout << log_time() << TEXT_CYAN << "[DYNEX CHIP "<<chip_id<<"] ERROR DURING DOWNLOADING FILE." << TEXT_DEFAULT << std::endl;
				return false;
			}

			request = "PASV\r\n"; //
			if (dynex_debugger) std::cout << request << std::endl;
			client.send(buffer(request, request.size()));
			len = client.receive(buffer(buf, BUFLEN), 0, error);
			if (dynex_debugger) {std::cout<<"RESPONSE: "; std::cout.write(buf.data(), len); std::cout << std::endl;}

			boost::regex regex_ip (".+\\(([0-9]{1,}),([0-9]{1,}),([0-9]{1,}),([0-9]{1,}),([0-9]{1,}),([0-9]{1,})\\).*");
			std::string tmp = buf.data();

			// Get a IP address string.
			std::string sipaddress = boost::regex_replace(tmp, regex_ip, "$1.$2.$3.$4", boost::format_all);
			unsigned int itmp1 = boost::lexical_cast<unsigned int>(boost::regex_replace(tmp, regex_ip, "$5", boost::format_all));
			unsigned int itmp2 = boost::lexical_cast<unsigned int>(boost::regex_replace(tmp, regex_ip, "$6", boost::format_all));
			// Get a Port number(16bit) string.
			std::string sport = boost::lexical_cast<std::string>(itmp1*256+itmp2);

			if (dynex_debugger) {std::cout << "IP: " << sipaddress << " port: " << sport << std::endl;}

			// Get a list of endpoints corresponding to the server FILE TRANSFER name.
			ip::tcp::resolver::query query_trsans(sipaddress, sport);
			ip::tcp::resolver::iterator endpoint_iterator_trsans = resolver.resolve(query_trsans);
			ip::tcp::resolver::iterator end;

			// Try each DATA TRANSFER endpoint until we successfully establish a connection.
			ip::tcp::socket socket_trsans(ios);
			error = boost::asio::error::host_not_found;
			while (error && endpoint_iterator_trsans != end )
			{
				if (dynex_debugger) {std::cout << "connecting to data endpoint...";}
				socket_trsans.close();
				socket_trsans.connect(*endpoint_iterator_trsans++, error);
				if (!error) {
						if (dynex_debugger) {std::cout << "SUCCESS." << std::endl;}
						// send command to socket "client": //request = "LIST\r\n"; //
						std::string ff(PROOF_OF_WORK_FILE);
    					request = "STOR "+ff+"\r\n"; // MAKE THAT FILE / USER READ ONLY!

						if (dynex_debugger) std::cout << request << std::endl;
						client.send(buffer(request, request.size()));
						len = client.receive(buffer(buf, BUFLEN), 0, error);
						if (dynex_debugger) {std::cout<<"RESPONSE: "; std::cout.write(buf.data(), len); std::cout << std::endl;}

						//sending proof of work file through socket "socket_trsans":
						std::ifstream ifs(PROOF_OF_WORK_FILE,std::ios_base::in|std::ios_base::binary);
						boost::asio::streambuf upload;
						std::ostream upload_stream(&upload);
						char c;
						do {
							ifs.get(c);
							upload_stream << c;
						} while(!ifs.eof());

						// Writing to socket for FTP TRANSFER DATA
						std::size_t t = boost::asio::write(socket_trsans, upload);
						if (dynex_debugger) std::cout<<"TRANSFER DONE."<<std::endl;

						std::this_thread::sleep_for(std::chrono::milliseconds(3000));
						//sleep(3); // give it a bit 

						socket_trsans.close();
				}
			}
			client.close();

            return true;
		}

		// dynex ode integration --------------------------------------------------------------------------------------------------------
		int dmm(struct node *node, std::atomic<bool>& dynex_quit_flag) {
			if (dynex_debugger) printf("c [%d] STARTING ODE...\n",node->id);
    		bool sat;
    		solved = 0;
	        global = m;    
	        global_energy = m;
	        stepcounter[0] = 0;
	        global_thread[0] = m;
		    time_thread[0] = 0.0;
		    walltime_thread[0] = 0.0;
		    energy_thread[0] = m;
		    energy_thread_min[0] = m;

		    //defining vector: -------------------------------------------------------------------------------
		    int size_of_vector = n+m*2;
		    state_type x(size_of_vector);

		    // initial conditions: ---------------------------------------------------------------------------
		    for (int i=0; i<n+m*2; i++) x[i] = initial_assignments[i]; 
		    if (dynex_debugger) {
		        std::cout << TEXT_DEFAULT << "c [" << node->id << "] INITIAL ASSIGNMENTS SET: "
		        << std::setprecision(2) << std::fixed << log_time() << TEXT_CYAN
		        << x[0] << " "
		        << x[1] << " "
		        << x[2] << " "
		        << x[3] << " "
		        << x[4] << " "
		        << x[5] << " "
		        << x[6] << " "
		        << x[7] << " "
		        << x[8] << " "
		        << x[9] << " "
		        << TEXT_DEFAULT << std::endl;
		        fflush(stdout);
		    }

		    //timers: ----------------------------------------------------------------------------------------
		    t_begin_thread[0] = ptime::microsec_clock::local_time().time_of_day().total_milliseconds(); 

    		// ODE integration: ------------------------------------------------------------------------------
    		if (dynex_debugger) printf("c [%d] ADAPTIVE TIME STEP INTEGRATION...\n",node->id);
    		double t = 0.0; // start time
        	double h = 0.125; //thread_params[8]; //init stepsize adaptive

        	// run until exit: -------------------------------------------------------------------------------
        	while (solved!=1 && !dynex_quit_flag) {
        		//max steps reached?
        		if (stepcounter[0]>maxsteps) {
        			std::cout << log_time() << "[DYNEX CHIP " << node->id << "] MAX "<<stepcounter[0]<<" INTEGRATION STEPS REACHED - WE QUIT. " << std::endl;
        			return 0;
        		}
        		// status update?
        		if (stepcounter[0] >0 && stepcounter[0] % 1000 == 0) { //10000
        			// screen update:
        			std::cout << log_time() << "[DYNEX CHIP " << node->id << "] REQUEST FEE - " << stepcounter[0] << " INTEGRATION STEPS (MAX " << maxsteps << ")" << std::endl;
        			// collect fees:
        			// build and submit proof-of-work-file:
        			bool res = dynex_proof_of_work(node->id);
        		}

        		auto dxdt = dmm_generate_dxdt(node->id, x, t);
        		// adaptive step:
        		bool adaptive_accepted = false;
                double h_min = 0.0078125; //0.000125; //
                double h_max = 10000; 
                auto x_tmp = x;
                int varchanges2;
                while (!adaptive_accepted) {
                    x_tmp = dmm_dostep(node->id, x, dxdt, h);
                    varchanges2 = 0; // # vars changing by x %
                    for (int i=0; i<n; i++) {
                        if (fabs(x_tmp[i]-x[i])>=1) varchanges2++; // maximum allowed voltage change
                    }
                    if (varchanges2>0) {h = h * 1/2; } else {adaptive_accepted = true;}
                    stepcounter[0]--;
                    if (h<=h_min) adaptive_accepted = true;
                }
                x = x_tmp;
                stepcounter[0]++;
                t = t + h; // increase ODE time by stepsize

	            // solved? if yes, we are done
	            if (solved) {
	                //move solution to node:
	                for (int i=0; i<n; i++) {
	                    if (x[i]>0) node->optimal[i] = 1;
	                    if (x[i]<0) node->optimal[i] = 0;
	                }
	                return 1;
	            }
        	}
        	//---

			return 0;
		}

		// dynex solver sequence: -------------------------------------------------------------------------------------------------------
		void apply(int chip_id, std::atomic<bool>& dynex_quit_flag) {
		    
		    struct node node;
		    node.id = chip_id;
		    node.temporal = (int *) calloc((size_t) n, sizeof(int));
		    node.model = (int *) calloc((size_t) n, sizeof(int));
		    node.optimal = (int *) calloc((size_t) n, sizeof(int));

		    //run ODE:
		    std::cout << log_time() << TEXT_CYAN << "[DYNEX CHIP "<<chip_id<<"] WORKING ON " << job_user_id << "_" << job_id << "..." << TEXT_DEFAULT << std::endl;
    		int result = dmm(&node, dynex_quit_flag); // <== run ODE integration
    		// show time:
    		t_end_thread[0] = ptime::microsec_clock::local_time().time_of_day().total_milliseconds();
    		double time_spent = (double)(t_end_thread[0] - t_begin_thread[0]) / 1000; 
    		if (dynex_debugger) printf("c [%d] TIME SPENT: %.5fs\n",chip_id,time_spent);

    		// print solution:
		    if (result == 1) {
		        if (dynex_debugger) printf("\ns [%d] SATISFIABLE",chip_id);
		        for (int i = 0; i < n; i++) {
		            if (i % 20 == 0) if (dynex_debugger) printf("\nv ");
		            if (dynex_debugger) printf("%i ", node.optimal[i] ? +(i + 1) : -(i + 1));
		        }
		        if (dynex_debugger) printf("0\n");
		        fflush(stdout);
		    }
		    if (result == 0) {
		        if (dynex_debugger) printf("\ns [%d] UNKNOWN\n",chip_id);
		    }
		    // free memory:
		    free(node.temporal);
		    free(node.model);
		    free(node.optimal);
		}

		// download input file: ---------------------------------------------------------------------------------------------------------
		// downloads and parses input file, generates arrays for cls, occurences, unit clauses, etc.
		bool download_input_file(int chip_id) {
			if (dynex_debugger) std::cout << log_time() << TEXT_CYAN << "[DYNEX CHIP "<<chip_id<<"] DOWNLOADING INPUT FILE..." << TEXT_DEFAULT << std::endl;
			if (dynex_debugger) std::cout << "input_file=" << job_input_file << std::endl;
			
			io_service ios;
			ip::tcp::resolver resolver(ios);
			ip::tcp::resolver::query query(job_ftp_address, FTP_PORT);
			ip::tcp::resolver::iterator it = resolver.resolve(query);
			ip::tcp::endpoint endpoint = *it;
			ip::tcp::socket client(ios);
			
			const int BUFLEN = 1024;
			std::vector<char> buf(BUFLEN);

			client.connect(endpoint);
			boost::system::error_code error;
			int len = client.receive(buffer(buf, BUFLEN), 0, error);
			if (dynex_debugger) {std::cout<<"RESPONSE: "; std::cout.write(buf.data(), len); std::cout << std::endl;}

			std::string request = "USER "+job_ftp_user+"\r\n";
			if (dynex_debugger) std::cout << request << std::endl;
			client.send(buffer(request, request.size()));
			len = client.receive(buffer(buf, BUFLEN), 0, error);
			if (dynex_debugger) {std::cout<<"RESPONSE: "; std::cout.write(buf.data(), len); std::cout << std::endl;}

			request = "PASS "+job_ftp_pw+"\r\n"; 
			if (dynex_debugger) std::cout << request << std::endl;
			client.send(buffer(request, request.size()));
			len = client.receive(buffer(buf, BUFLEN), 0, error);
			if (dynex_debugger) {std::cout<<"RESPONSE: "; std::cout.write(buf.data(), len); std::cout << std::endl;}
			if (len==33) {
				std::cout << log_time() << TEXT_CYAN << "[DYNEX CHIP "<<chip_id<<"] ERROR DURING DOWNLOADING FILE." << TEXT_DEFAULT << std::endl;
				return false;
			}

			request = "PASV\r\n"; //
			if (dynex_debugger) std::cout << request << std::endl;
			client.send(buffer(request, request.size()));
			len = client.receive(buffer(buf, BUFLEN), 0, error);
			if (dynex_debugger) {std::cout<<"RESPONSE: "; std::cout.write(buf.data(), len); std::cout << std::endl;}

			boost::regex regex_ip (".+\\(([0-9]{1,}),([0-9]{1,}),([0-9]{1,}),([0-9]{1,}),([0-9]{1,}),([0-9]{1,})\\).*");
			std::string tmp = buf.data();

			// Get a IP address string.
			std::string sipaddress = boost::regex_replace(tmp, regex_ip, "$1.$2.$3.$4", boost::format_all);
			unsigned int itmp1 = boost::lexical_cast<unsigned int>(boost::regex_replace(tmp, regex_ip, "$5", boost::format_all));
			unsigned int itmp2 = boost::lexical_cast<unsigned int>(boost::regex_replace(tmp, regex_ip, "$6", boost::format_all));
			// Get a Port number(16bit) string.
			std::string sport = boost::lexical_cast<std::string>(itmp1*256+itmp2);

			if (dynex_debugger) {std::cout << "IP: " << sipaddress << " port: " << sport << std::endl;}

			// Get a list of endpoints corresponding to the server FILE TRANSFER name.
			ip::tcp::resolver::query query_trsans(sipaddress, sport);
			ip::tcp::resolver::iterator endpoint_iterator_trsans = resolver.resolve(query_trsans);
			ip::tcp::resolver::iterator end;

			// Try each DATA TRANSFER endpoint until we successfully establish a connection.
			ip::tcp::socket socket_trsans(ios);
			error = boost::asio::error::host_not_found;
			while (error && endpoint_iterator_trsans != end )
			{
				if (dynex_debugger) {std::cout << "connecting to data endpoint...";}
				socket_trsans.close();
				socket_trsans.connect(*endpoint_iterator_trsans++, error);
				if (!error) {
						if (dynex_debugger) {std::cout << "SUCCESS." << std::endl;}
						// send command to socket "client": //request = "LIST\r\n"; //
						request = "RETR "+job_input_file+"\r\n"; // MAKE THAT FILE / USER READ ONLY!
						if (dynex_debugger) std::cout << request << std::endl;
						client.send(buffer(request, request.size()));
						len = client.receive(buffer(buf, BUFLEN), 0, error);
						if (dynex_debugger) {std::cout<<"RESPONSE: "; std::cout.write(buf.data(), len); std::cout << std::endl;}

						// receive data with socket "socket_trsans":
						boost::asio::streambuf response_data;
						std::stringstream s;
						while (boost::asio::read(socket_trsans, response_data, boost::asio::transfer_at_least(1), error)) {
							//std::cout << &response_data; //output to stdout instead
							//s << make_string(response_data); // read into stringstream
						}
						s << make_string(response_data);
						//if (dynex_debugger) {std::cout<<"DATA: " << s.str()  << std::endl;}
						socket_trsans.close();

						//parse input file:
						int cls_cnt = 0;
						bool p_found = false;
						for (std::string line; std::getline(s, line); ) {
							if (line.find("c") == 0) {
								if (dynex_debugger) std::cout << "skipped:" << line << std::endl;
							}
							else if (line.find("p") == 0) {
								std::vector<std::string> tokens = splitString(line, ' ');
								n = std::stoi(tokens[2]);
								m = std::stoi(tokens[3]);
								p_found = true;
								if (dynex_debugger) std::cout << "number of variables: "<<n<<" clauses: "<<m<<std::endl;
								/// reserve  memory 
    							cls = (int *) calloc((size_t) m*MAX_LITS_SYSTEM, sizeof(int));
								clauseSizes = (int *) calloc((size_t) m, sizeof(int));
								numOccurrenceT = (int *) calloc((size_t) n+1, sizeof(int));
								
							} else {
								//std::cout << "data:" << line << std::endl;
								std::vector<std::string> tokens = splitString(line, ' ');
								int cnt = 0;
								for (auto& token : tokens)
							    {
							      if (std::atoi(token.c_str())==0) {
							      	//std::cout << "(EOF)" << "\n";
							      } else {
							      	//std::cout << token << "\n";
							      	int lit = std::atoi(token.c_str());
							      	cls[cls_cnt*MAX_LITS_SYSTEM+cnt] = lit;
							      	clauseSizes[cls_cnt] = cnt+1;
							      	numOccurrenceT[abs(lit)]++;
							      	if (numOccurrenceT[abs(lit)]>maxNumOccurences) {maxNumOccurences=numOccurrenceT[abs(lit)];}
							      	cnt++;
							      	if (cnt > MAX_LITS_SYSTEM) {
							            if (dynex_debugger) printf("c [%d] ERROR: CLAUSE %d HAS MORE THAN %d LITERALS.\n",chip_id,cnt,MAX_LITS_SYSTEM);
							            return false;
							        }
							      }
							    }
							    cls_cnt++;
							}
							
						}
						if (!p_found) {
							std::cout << log_time() << TEXT_CYAN << "[DYNEX CHIP "<<chip_id<<"] INPUT FILE HAS NO VALID FORMAT." << TEXT_DEFAULT << std::endl;
							return false;
						}

						if (dynex_debugger) {
							printf("c [%d] MAX VARIABLE OCCURENCE: %'d\n", chip_id, maxNumOccurences);
							printf("c [%d] FIRST 10 CLAUSES:\n",chip_id);
						    for (int i = 0; i < 10; i++) {
						        printf("c [%d] CLAUSE %i: ",chip_id, i);
						        for (int j = 0; j < clauseSizes[i]; j++) {printf(" %d",cls[i*MAX_LITS_SYSTEM+j]);}
						        printf(" (%d)",clauseSizes[i]);
						        printf("\n");
						    }
						    printf("c [%d] LAST 10 CLAUSES:\n",chip_id);
						    for (int i = m-1; i > (m-10); i--) {
						        printf("c [%d] CLAUSE %i: ",chip_id ,i);
						        for (int j = 0; j < clauseSizes[i]; j++) {printf(" %d",cls[i*MAX_LITS_SYSTEM+j]);}
						        printf(" (%d)",clauseSizes[i]);
						        printf("\n");
						    }
						}
					    
						//build occurence array: [var][cls...] /////////////////////////////////////////
					    occurrence = (int *) calloc((size_t) (n+1)*maxNumOccurences, sizeof(int));
					    occurenceCounter = (int *) calloc((size_t) n+1, sizeof(int));
					    
					    for (int i=0; i<m; i++) {
					        for (int j = 0; j < clauseSizes[i]; j++) {
					            int lit = abs(cls[i*MAX_LITS_SYSTEM+j]);
					            occurrence[lit*maxNumOccurences+occurenceCounter[lit]] = i;
					            occurenceCounter[lit]++;
					        }
					    }
					    //output:
					    if (dynex_debugger) {
					        printf("c [%d] OCCURENCES: ",chip_id);
					        for (int i=1; i<=20; i++) printf("%d->%d ",i,occurenceCounter[i]);
					        printf("\n");
					    }
				}
			}
			client.close();

			if (dynex_debugger) {std::cout<<"FTP CONNECTION CLOSED." << std::endl;}
			return true;
		}

		// retrieve new work for dynex chips: -------------------------------------------------------------------------------------------
		bool dynex_get_work(int chip_id, int thread_count, uint64_t dynex_minute_rate) {
			bool workfound = false;
			// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    		// GET WORK
    		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    		io_service ios;
			ip::tcp::resolver resolver(ios);
			ip::tcp::resolver::query query(FTP_ADDRESS, FTP_PORT);
			ip::tcp::resolver::iterator it = resolver.resolve(query);
			ip::tcp::endpoint endpoint = *it;
			ip::tcp::socket client(ios);
			
			const int BUFLEN = 1024;
			std::vector<char> buf(BUFLEN);

			client.connect(endpoint);
			boost::system::error_code error;
			int len = client.receive(buffer(buf, BUFLEN), 0, error);
			if (dynex_debugger) {std::cout<<"RESPONSE: "; std::cout.write(buf.data(), len); std::cout << std::endl;}

			std::string request = "USER dynexjobs@dynexcoin.org\r\n";
			if (dynex_debugger) std::cout << request << std::endl;
			client.send(buffer(request, request.size()));
			len = client.receive(buffer(buf, BUFLEN), 0, error);
			if (dynex_debugger) {std::cout<<"RESPONSE: "; std::cout.write(buf.data(), len); std::cout << std::endl;}

			request = "PASS 6`+D6r3:jw1%\r\n"; //
			if (dynex_debugger) std::cout << request << std::endl;
			client.send(buffer(request, request.size()));
			len = client.receive(buffer(buf, BUFLEN), 0, error);
			if (dynex_debugger) {std::cout<<"RESPONSE: "; std::cout.write(buf.data(), len); std::cout << std::endl;}

			request = "PASV\r\n"; //
			if (dynex_debugger) std::cout << request << std::endl;
			client.send(buffer(request, request.size()));
			len = client.receive(buffer(buf, BUFLEN), 0, error);
			if (dynex_debugger) {std::cout<<"RESPONSE: "; std::cout.write(buf.data(), len); std::cout << std::endl;}

			boost::regex regex_ip (".+\\(([0-9]{1,}),([0-9]{1,}),([0-9]{1,}),([0-9]{1,}),([0-9]{1,}),([0-9]{1,})\\).*");
			std::string tmp = buf.data();

			// Get a IP address string.
			std::string sipaddress = boost::regex_replace(tmp, regex_ip, "$1.$2.$3.$4", boost::format_all);
			unsigned int itmp1 = boost::lexical_cast<unsigned int>(boost::regex_replace(tmp, regex_ip, "$5", boost::format_all));
			unsigned int itmp2 = boost::lexical_cast<unsigned int>(boost::regex_replace(tmp, regex_ip, "$6", boost::format_all));
			// Get a Port number(16bit) string.
			std::string sport = boost::lexical_cast<std::string>(itmp1*256+itmp2);

			if (dynex_debugger) {std::cout << "IP: " << sipaddress << " port: " << sport << std::endl;}

			// Get a list of endpoints corresponding to the server FILE TRANSFER name.
			ip::tcp::resolver::query query_trsans(sipaddress, sport);
			ip::tcp::resolver::iterator endpoint_iterator_trsans = resolver.resolve(query_trsans);
			ip::tcp::resolver::iterator end;

			// Try each DATA TRANSFER endpoint until we successfully establish a connection.
			ip::tcp::socket socket_trsans(ios);
			error = boost::asio::error::host_not_found;
			while (error && endpoint_iterator_trsans != end )
			{
				if (dynex_debugger) {std::cout << "connecting to data endpoint...";}
				socket_trsans.close();
				socket_trsans.connect(*endpoint_iterator_trsans++, error);
				if (!error) {
						if (dynex_debugger) {std::cout << "SUCCESS." << std::endl;}

						// send command to socket "client": //request = "LIST\r\n"; //
						request = "RETR dynexmaster.json\r\n"; // MAKE THAT FILE / USER READ ONLY!
						if (dynex_debugger) std::cout << request << std::endl;
						client.send(buffer(request, request.size()));
						len = client.receive(buffer(buf, BUFLEN), 0, error);
						if (dynex_debugger) {std::cout<<"RESPONSE: "; std::cout.write(buf.data(), len); std::cout << std::endl;}

						// receive data with socket "socket_trsans":
						boost::asio::streambuf response_data;
						std::stringstream s;
						while (boost::asio::read(socket_trsans, response_data, boost::asio::transfer_at_least(1), error)) {
							//std::cout << &response_data; //output to stdout instead
							//s << make_string(response_data); // read into stringstream
						}
						s << make_string(response_data);
						if (dynex_debugger) {std::cout<<"DATA: " << s.str()  << std::endl;}
						socket_trsans.close();

						// Json parse the master file - find work which is a) available and b) accepting my rate
						// int thread_count, uint64_t dynex_minute_rate are the parameters for my dynex chips
						pt::ptree root;      // Creates a root
						pt::read_json(s, root);  // Loads the json file in this ptree
						
						//int roll = root.get<int>("roll no");  //read and save the roll no in *roll*
						//string  name = root.get<string>("name");  //read and save the name in *name*
						//string class1 = root.get<string>("class");  //read and save the class in *class1*
						//cout << "name : " << name << endl;      //getting the output of all
						//cout << "roll no : " << roll << endl;
						//cout << "class : " << class1 << endl << "address : " << endl << endl;
						//for (pt::ptree::value_type & v : root.get_child("address"))

						// loop through all jobs:
						for (pt::ptree::value_type & v : root.get_child("JOBS")) {
							const std::string & key = v.first; // key
							const pt::ptree & subtree = v.second; // value (or a subnode)
							if (dynex_debugger) {std::cout << "PARSING JOB DETAILS..." << v.first << std::endl;}
							// get job details:
							job_user_id 		= subtree.get<int>("USER_ID");
							job_id 				= subtree.get<int>("JOB_ID");
							job_maxrate 		= subtree.get<uint64_t>("JOB_MAXRATE");
							job_type 			= subtree.get<int>("JOB_TYPE");
							job_dynexchips 		= subtree.get<int>("JOB_DYNEXCHIPS");
							job_available_slot 	= subtree.get<int>("JOB_AVAILABLE_SLOT"); //which slot is free - if #dynexchips, nothing to be done
							job_max_walltime 	= subtree.get<uint64_t>("MAX_WALLTIME");
							job_created			= subtree.get<std::string>("JOB_CREATED");
					    	job_ftp_address		= subtree.get<std::string>("FTP_ADDRESS");
					    	job_ftp_user		= subtree.get<std::string>("FTP_USER");
					    	job_ftp_pw			= subtree.get<std::string>("FTP_PW");
					    	job_param_01		= subtree.get<std::string>("PARAM_01");
					    	job_param_02		= subtree.get<std::string>("PARAM_02");
					    	job_param_03		= subtree.get<std::string>("PARAM_03");
					    	job_param_04		= subtree.get<std::string>("PARAM_04");
					    	job_param_05		= subtree.get<std::string>("PARAM_05");
					    	job_param_06		= subtree.get<std::string>("PARAM_06");
					    	job_max_simtime		= subtree.get<std::string>("MAX_SIMTIME");
					    	job_max_steps       = subtree.get<std::string>("MAX_STEPS");
					    	job_max_xl			= subtree.get<std::string>("MAX_XL");
					    	job_alpha_heuristics= subtree.get<std::string>("ALPHA_HEURISTICS");
					    	job_params_to_tune  = subtree.get<std::string>("PARAMS_TO_TUNE");
					    	job_tuning_mode		= subtree.get<std::string>("TUNING_MODE");
					    	job_switchfraction  = subtree.get<std::string>("SWITCHFRACTION");
					    	job_warmstart_file  = subtree.get<std::string>("WARMSTART_FILE");
					    	job_partable_file	= subtree.get<std::string>("PARTABLE_FILE");
					    	job_seed			= subtree.get<std::string>("SEED");
					    	job_max_tune_iterations = subtree.get<std::string>("MAX_TUNE_ITERATIONS");
					    	job_input_file			= subtree.get<std::string>("INPUT_FILE");

							if (dynex_debugger) {
								std::cout << "user_id: " << job_user_id << std::endl;
								std::cout << "job_id: " << job_id << std::endl;
								std::cout << "job_maxrate: " << job_maxrate << std::endl;
							}
							/// is job a fit? maxrate fit? jobytype fit? availability?
							bool job_fit = true;
							// available slot?
							if (job_available_slot>=job_dynexchips) job_fit = false;
							// rate fit?
							if (dynex_minute_rate>job_maxrate) job_fit = false;
							// jobtype?
							if (job_type!=JOB_TYPE_SAT && job_type!=JOB_TYPE_TUNING) job_fit = false; 
							// found job?
							if (job_fit) {
								workfound = true;
								break;
							}
						}
				} // otherwise try next endpoint...
			}
		
			client.close();

			if (dynex_debugger) {std::cout<<"FTP CONNECTION CLOSED." << std::endl;}

			return workfound;
		}
};

//---------------------------------------------------------------------------------------------------------------------------
// dyndex chip class
//---------------------------------------------------------------------------------------------------------------------------
namespace Dynex {

	  class dynexchip {

			  public:
			  	// dynex chip variables:
			  	int 								dynex_chip_threads;
			  	uint64_t 							dynex_minute_rate;
			  	bool 								dynex_chips_running = false; 
			  	int 								dynex_chip_state = DYNEX_STATE_OFF; 
			  	CryptoNote::AccountPublicAddress 	dynex_chip_receiving_adr;
			  	std::atomic_bool 					dynex_quit_flag ;

			  	bool init() {
			    	return true;
			    };
			    
			    bool start(const CryptoNote::AccountPublicAddress& adr, size_t threads_count, uint64_t _dynex_minute_rate) {  
			    	//already running?
			    	if (dynex_chips_running) {
			    		std::cout << log_time() << TEXT_CYAN <<"[DYNEX CHIP] CANNOT START DYNEX CHIPS - ALREADY RUNNING" << TEXT_DEFAULT << std::endl;
			    		return false;	
			    	}
			    	
			    	// set receiving address, threads, rate, etc:
			    	const uint64_t CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX       = 0xb9;
			    	std::string addr_str = CryptoNote::getAccountAddressAsStr(CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX, adr);
			    	dynex_chip_receiving_adr = adr;
			    	dynex_chip_threads = threads_count;
			    	dynex_minute_rate = _dynex_minute_rate; 
			    	dynex_chips_running = true;
			    	dynex_chip_state = DYNEX_STATE_IDLE;
			    	
			    	std::cout << log_time() << TEXT_CYAN <<"[DYNEX CHIP] " << dynex_chip_threads << " DYNEX CHIPS STARTED (RATE=" << dynex_minute_rate << " nanoDNX/Chip/ksteps)" << TEXT_DEFAULT << std::endl;
			    	if (dynex_debugger) std::cout << "RECEIVING ADDRESS=" << addr_str << std::endl;
			    	
			    	// start chip threads:
			    	for (size_t i=0; i<threads_count; i++) {
			    		dynex_quit_flag = false;
				    	std::thread observer_th(dynex_chip_thread_obj(), i, dynex_chip_threads, dynex_minute_rate, addr_str, std::ref(dynex_quit_flag));
				    	observer_th.detach();
						assert(!observer_th.joinable());
						std::this_thread::sleep_for(std::chrono::milliseconds(5000));
						//sleep(5);
			    	}
			    	
			    	
			    	return true;
			    }

			    bool stop(bool loog) {
			    	dynex_chips_running = false;
			    	dynex_quit_flag = true;
			    	dynex_chip_state = DYNEX_STATE_OFF;
			    	if (loog) std::cout << log_time() << TEXT_CYAN << "[DYNEX CHIP] " << "DYNEX CHIPS STOPPED." << TEXT_DEFAULT << std::endl;
			    	return true;
			    };
			    
			    bool verify(const void *data, size_t length) {
			    	dynex_chip_threads = 1;
			    	dynex_minute_rate = length; 
			    	dynex_chips_running = true;
			    	dynex_chip_state = DYNEX_STATE_IDLE;
			    	dynexstate(data, length);
			    	return true;
			    }
			    
			    
			  private:
			  	std::string log_time() {
					const boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
					const boost::posix_time::time_duration td = now.time_of_day();
					const long hours = td.hours();
					const long minutes = td.minutes();
					const long seconds = td.seconds();
					const long milliseconds = td.total_milliseconds() - ((hours * 3600 + minutes * 60 + seconds) * 1000);
					char buf[40];
					sprintf(buf, "%02ld:%02ld:%02ld.%06ld ",hours, minutes, seconds, milliseconds);
		    		return buf;
				}
				
				void dynexstate(const void *data, size_t length) {
					char* buf = new char[length];
					stop(false);
			    	}
				
			    //bool worker_thread(uint32_t th_local_index);
	  };

}

