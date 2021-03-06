//	
//	This tool can be used to achieve likelihood ratio for suspect(s) and/or victim(s) in a DNA mixture using probabilistic genotyping
//	re-engineered Quant-Based Tool (C) 2015 Munieshwar Ramdass, Nicholas J. Corpuz, Khagay Nagdimov
//	
//	This program is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//	GNU General Public License for more details.
//	
//	You should have received a copy of the GNU General Public License
//	along with this program. If not, see <http://www.gnu.org/licenses/>.
//

//
//	re-engineered Quant-Based Tool (reQBT)
//	
//	Team:	Clinton Hughes		(Supervisor)
//			Munieshwar Ramdass	(C++ Programmer)
//			Nicolas Corpus		(GUI, R Programmer)
//			Khaguy Nagdimov		(GUI, R Programmer)
//			
//	June 2nd, 2015 - August 14th, 2015
//

//
//	Application security has not been done - Will rely on an external program/person to check file format and exceptions
//	Simple procedures like checking for const methods or variables, or passing by references may not have been done
//	Code was written to work without regards to runtime or performance - Time costing transposing of matrix was done
//	Unsafe changing of global variables was done
//
//	Entire coding logic developed by Munieshwar Ramdass and can be explained in detail by him
//	Design of user interface done in R by Nick Corpuz and Khagay Nagdimov and can be explained by them
//	Any questions concerning the programs discussed above contact the team at: LAST.help.questions@gmail.com
//	

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <ctime>
#include <cstdlib>
#include <list>
#include <set>
#include <algorithm>
#include <iterator>
#include <functional>
#include <limits>
#include <cassert>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdarg.h>

typedef std::string String;								// The following typedef used for CSV as a means of convinience
typedef std::vector<String> csv_row;
typedef std::vector<csv_row> csv_database;

using namespace std;

// Parameters: None
class timer {
public:
	timer() : start(clock()) {}
	double elapsed() { return (clock() - start) / CLOCKS_PER_SEC; }
	void reset() { start = clock(); }
private:
	double start;
};


///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Calculations based on report
// Allele, Genotype Frequency, Popukation generation
// Drop out/in analysis
//
//

int FILE_NUM = 1;
int RACE = 1;
double HOM_CONST = 0.00;	// inbreed - THETA

double PC0 = 0.0;			// Drop-in rates
double PC1 = 0.0;			// Not constant
double PC2 = 0.0;

double PHET0 = 0.0;			// Drop-out rates
double PHET1 = 0.0;			// Not constant
double PHET2 = 0.0;
double PHOM0 = 0.0;
double PHOM1 = 0.0;

bool DEDUCIBLE = false;
double QUANT = 25.0;

double MINIMUM_WILD_FREQUENCY = 0.001;	// Not constant

class Genotype_Combination;
class Person;
class Report;

// Parameters: string locus, double length, double freq, bool flag
class Allele {
public:
	Allele(string locus, double length, double freq, bool flag)
		:locus(locus), length(length), freq(freq), flag(flag) {}

	bool operator==(const Allele& right) const {
		return (locus == right.locus && length == right.length && freq == right.freq && flag == right.flag);
	}

	bool operator!=(const Allele& right) const {
		return !(*this == right);
	}

	// GETTERs and SETTERS

	string get_locus() const { return locus; }

	double get_length() const { return length; }

	double get_freq() const { return freq; }

	bool get_flag() const { return flag; }

private:
	string locus;
	double length;
	double freq;
	bool flag;

	friend Genotype_Combination;
	friend Person;
	friend Report;
};

// Parameter: vector<Allele>& a
class Genotype_Combination {
public:
	Genotype_Combination(vector<Allele>& a)
		: flag(true) {

		if (a.size() == 0) {
			Allele tmp_a("Wild", -1, 1.0, true);
			alleles.push_back(tmp_a);
		}
		else {
			for (int i(0); i < a.size(); ++i) {
				alleles.push_back(a[i]);
			}
		}

		generate_allele_pairs();
		generate_freqs();
	}

	void print_alleles() {		// Not used
		for (int i(0); i < alleles.size(); ++i) {
			cout << "\tLocus:\t\t" << alleles[i].locus
				<< "\n\tLength:\t\t" << alleles[i].length << "\n\tFrequency:\t" << alleles[i].freq << endl;
		}
	}

	void print_freqs() {		// Not used
		for (int i(0); i < freqs.size(); ++i) {
			cout << "\tCombinations: " << i + 1 << " " << freqs[i] << endl;
		}
	}

	void generate_freqs() {
		for (int i(0); i < alleles.size(); ++i) {
			for (int j(i); j < alleles.size(); ++j) {
				if (i == j) {
					freqs.push_back(calc_hom(alleles[i]));
				}
				else {
					freqs.push_back(calc_het(alleles[i], alleles[j]));
				}
			}
		}
	}

	void generate_allele_pairs() {
		for (int i(0); i < alleles.size(); ++i) {
			for (int j(i); j < alleles.size(); ++j) {
				allele_comb.push_back(pair<Allele, Allele>(alleles[i], alleles[j]));
			}
		}
	}

	double calc_hom(Allele a) const {
		return a.freq * a.freq + a.freq * HOM_CONST * (1 - a.freq);
	}

	double calc_het(Allele a, Allele b) const {
		return 2 * a.freq * b.freq;
	}


private:
	vector<Allele> alleles;
	vector<pair<Allele, Allele>> allele_comb;
	vector<double> freqs;
	bool flag;

	friend Allele;
	friend Person;
	friend Report;
};

// Parameters: string name, Allele a, Allele b
class Person {
public:
	Person(string name, Allele a, Allele b)
		: name(name), a(a), b(b) {
		generate_freq();
		if (a == b) {
			hom = true;
			het = false;
		}
		else {
			hom = false;
			het = true;
		}
	}

	void print_alleles() {
		cout << name << "'s Alleles:" << endl;
		cout << "First Allele:" << endl;
		cout << "\tlocus:\t\t\t" << a.locus
			<< "\n\tLength:\t\t\t" << a.length << "\n\tFrequency:\t\t" << a.freq << endl;
		cout << "Second Allele:" << endl;
		cout << "\tlocus:\t\t\t" << b.locus
			<< "\n\tLength:\t\t\t" << b.length << "\n\tFrequency:\t\t" << b.freq << endl;
		cout << "\tGenotype Frequency:\t" << freq << endl;
	}

	void generate_freq() {
		if (a == b) {
			freq = calc_hom(a);
		}
		else {
			freq = calc_het(a, b);
		}
	}

	double calc_hom(Allele a) const {
		return a.freq * a.freq + a.freq * HOM_CONST * (1 - a.freq);
	}

	double calc_het(Allele a, Allele b) const {
		return 2 * a.freq * b.freq;
	}

	bool operator==(const Person& right) const {
		return (a == right.a && b == right.b && freq == right.freq && hom == right.hom && het == right.het);
	}
	bool operator!=(const Person& right) const {
		return !(*this == right);
	}

	// GETTERS and SETTERS

	string get_name() { return name; }

	Allele get_a() { return a; }

	Allele get_b() { return b; }

private:
	string name;
	Allele a;
	Allele b;
	double freq;
	bool hom;
	bool het;

	friend Allele;
	friend Genotype_Combination;
	friend Report;
};

// Parameters: int unknowns, Person s, Genotype_Combination g, vector<vector<Allele>>& a
class Report {
public:
	Report(int unknowns, vector<Person> s, vector<Person> d, Genotype_Combination g, vector<vector<Allele>>& a)
		: flag(true), rep_num(0), s(s), d(d), g(g), unknowns(unknowns) {

		for (int i(0); i < unknowns; ++i) {
			vector<Person> p;
			population.push_back(p);
		}

		for (int i(0); i < a.size(); ++i) {
			rep_alleles.push_back(a[i]);
			++rep_num;
		}

		vector<int> tmp;
		for (int i(0); i < g.allele_comb.size(); ++i) {
			tmp.push_back(i);
		}
		if (unknowns > 0 && unknowns <= tmp.size()) {
			permute_vector_driver(tmp, unknowns);
		}
		else {
			indicies.push_back(tmp);
		}

		generate_persons();
		generate_population();
		for (int i(0); i < population[0].size(); ++i) {
			vector<Person> p;
			population_t.push_back(p);
		}
		transpose(population, population_t);

		vector<Person> wild_vector;
		if (population_t.size() == 1) {
			population_t.clear();
			for (int i(0); i < unknowns; ++i)
				wild_vector.push_back(persons[0]);
			population_t.push_back(wild_vector);
		}

		check_person();

		ofstream ofs, ofs2, ofs3;
		ofs.open("Evidence_" + to_string(FILE_NUM) + ".csv", ios::app);
		ofs2.open("output.csv", ios::app);
		ofs3.open("output.txt", ios::app);

		if (RACE == 1) {
			ofs << "\nBLACK\n";
			ofs2 << "\nBLACK\n";
			ofs3 << "\nBLACK\n";
		}
		else if (RACE == 2) {
			ofs << "\nCAUCASIAN\n";
			ofs2 << "\nCAUCASIAN\n";
			ofs3 << "\nCAUCASIAN\n";
		}
		else if (RACE == 3) {
			ofs << "\nHISPANIC\n";
			ofs2 << "\nHISPANIC\n";
			ofs3 << "\nHISPANIC\n";
		}
		else if (RACE == 4) {
			ofs << "\nASIAN\n";
			ofs2 << "\nASIAN\n";
			ofs3 << "\nASIAN\n";
		}

		ofs << ",HET0:," << PHET0 << ",,pC0:," << PC0 << "\n,HET1:," << PHET1 << ",,pC1:," << PC1
			<< "\n,HET2:," << PHET2 << ",,pC2:," << PC2 << "\n,HOM0:," << PHOM0 << "\n,HOM1:," << PHOM1
			<< ",,Theta:," << HOM_CONST << endl;
		ofs2 << ",HET0:," << PHET0 << ",,pC0:," << PC0 << "\n,HET1:," << PHET1 << ",,pC1:," << PC1
			<< "\n,HET2:," << PHET2 << ",,pC2:," << PC2 << "\n,HOM0:," << PHOM0 << "\n,HOM1:," << PHOM1
			<< ",,Theta:," << HOM_CONST << endl;
		ofs3 << "\tHET0:\t" << PHET0 << "\t\tpC0:\t" << PC0 << "\n\tHET1:\t" << PHET1 << "\t\tpC1:\t" << PC1
			<< "\n\tHET2:\t" << PHET2 << "\t\tpC2:\t" << PC2 << "\n\tHOM0:\t" << PHOM0 << "\n\tHOM1:\t" << PHOM1
			<< "\t\tTheta:\t" << HOM_CONST << endl;

		ofs << ",\nLocus:," << s[0].a.locus << ",,Quant:," << QUANT << ",,Deducible:," << DEDUCIBLE << endl;
		ofs2 << ",\nLocus:," << s[0].a.locus << ",,Quant:," << QUANT << ",,Deducible:," << DEDUCIBLE << endl;
		ofs3 << "\nLocus:\t" << s[0].a.locus << "\t\tQuant:\t" << QUANT << "\t\tDeducible:\t" << DEDUCIBLE << endl;

		for (int i(0); i < s.size(); ++i) {
			ofs << "Known:," << s[i].a.length << "," << s[i].b.length << ",," << s[i].name << endl;
			ofs2 << "Known:," << s[i].a.length << "," << s[i].b.length << ",," << s[i].name << endl;
			ofs3 << "\nKnown:\t" << s[i].a.length << ", " << s[i].b.length << "\t" << s[i].name << endl;
		}

		ofs << "\nGenotype Freq" << endl;
		ofs2 << "\nGenotype Freq" << endl;
		ofs3 << "\nGenotype Freq" << endl;
		for (int i(0); i < persons.size(); ++i) {
			ofs << ",(" << persons[i].a.length << "," << persons[i].b.length << "),(" << persons[i].a.freq << "," << persons[i].b.freq << ")," << persons[i].freq << endl;
			ofs2 << ",(" << persons[i].a.length << "," << persons[i].b.length << "),(" << persons[i].a.freq << "," << persons[i].b.freq << ")," << persons[i].freq << endl;
			ofs3 << "\t" << persons[i].a.length << ", " << persons[i].b.length << "\t" << persons[i].a.freq << ", " << persons[i].b.freq << "\t" << persons[i].freq << endl;
		}

		cout << "\nNow Generating Sums..." << endl;
		generate_sums();

		LR = numerator_sum / denominator_sum;
		cout << "Likelihood Ratio:\t" << LR << endl;

		ofs << ",,LR:," << LR << ",,Pn:," << numerator_sum << ",,Pd:," << denominator_sum << endl;
		ofs2 << "\n,,LR:," << LR << ",,Pn:," << numerator_sum << ",,Pd:," << denominator_sum << endl;
		ofs3 << "\nLR:\t" << LR << "\tPn:\t" << numerator_sum << "\tPd:\t" << denominator_sum << endl;

		for (int i(0); i < persons.size() && i < LRs.size() && i < numerators.size(); ++i) {
			cout << persons[i].a.length << "\t" << persons[i].b.length << "\t" << LRs[i] << endl;
			ofs << "," << persons[i].a.length << "," << persons[i].b.length << "," << LRs[i] << ",,Pn:," << numerators[i] << ",,Pd:," << denominator_sum << endl;
			ofs2 << "," << persons[i].a.length << "," << persons[i].b.length << "," << LRs[i] << ",,Pn:," << numerators[i] << ",,Pd:," << denominator_sum << endl;
			ofs3 << "\t" << persons[i].a.length << ", " << persons[i].b.length << "\t" << LRs[i] << endl;
		}

		ofs.close();
		ofs2.close();
		ofs3.close();
	}

	void permute_vector(const vector<int>& number_items, unsigned int length, vector<unsigned int>& position, unsigned int depth, unsigned int perimeter) {
		if (depth >= length) {
			vector<int> index_vector;

			for (unsigned int i = 0; i < position.size(); ++i) {
				index_vector.push_back(number_items[position[i]]);
			}
			indicies.push_back(index_vector);
			return;
		}

		for (unsigned int i = 0; i < number_items.size(); ++i) { // CHANGE i TO perimeter TO MAKE THIS A COMBINATION WITH REPETITION
			position[depth] = i;
			permute_vector(number_items, length, position, depth + 1, i);
		}
		return;
	}

	void permute_vector_driver(const vector<int>& number_items, unsigned int length) {
		assert(length > 0 && length <= number_items.size());
		vector<unsigned int> positions(length, 0);
		permute_vector(number_items, length, positions, 0, 0);
	}

	void generate_population() {		// Generates all combinaions of genotypes
		for (int i(0); i < indicies.size(); ++i) {
			for (int j(0); j < indicies[i].size(); ++j) {
				Person temp("Unknown", g.allele_comb[indicies[i][j]].first, g.allele_comb[indicies[i][j]].second);
				population[j].push_back(temp);

			}
		}
	}

	bool is_subset(int ind) {
		if (equal(population_t[ind].begin(), population_t[ind].begin() + s.size(), s.begin()))
			return true;
		return false;
	}

	bool is_subset_d(int ind) {	// Verson of subset to test on denominator
		if (equal(population_t[ind].begin(), population_t[ind].begin() + d.size(), d.begin()))
			return true;
		return false;
	}

	void generate_sums() {
		double sum(0.0);
		double prod(1.0);
		double s_sum(0.0);		// Sum for known person(s) only
		double s_prod(1.0);		// Product used for known person(s) only

		double sum_d(0.0);
		double prod_d(1.0);

		//ofstream ofs;
		//ofs.open("Evidence_" + to_string(FILE_NUM) + ".csv", ios::app);
		//ofs << endl;
		for (int ii(0); ii < persons.size(); ++ii) {
			for (int i(0); i < population_t.size(); ++i) {
				if (population_t[i][0] == persons[ii] && is_subset(i)) { // First if-statement from LASTv3
					for (int j(0); j < population_t[i].size(); ++j) {
						if (j >= unknowns - (unknowns - s.size())) {
							prod *= population_t[i][j].freq;
							s_prod = prod;
						}

						if (j >= unknowns - (unknowns - d.size()))
							prod_d *= population_t[i][j].freq;

						//ofs << population_t[i][j].freq << ",";
					}
					for (int j(0); j < population_t[i].size(); ++j) {
						for (int k(0); k < rep_alleles.size(); ++k) {
							prod *= drop_out(population_t[i][j], rep_alleles[k]);
							s_prod = prod;
							prod_d *= drop_out(population_t[i][j], rep_alleles[k]);

							//ofs << drop_out(population_t[i][j], rep_alleles[k]) << ",";
						}
					}
					for (int j(0); j < rep_alleles.size(); ++j) {
						prod *= drop_in(population_t[i], rep_alleles[j]);
						s_prod = prod;
						prod_d *= drop_in(population_t[i], rep_alleles[j]);

						//ofs << drop_in(population_t[i], rep_alleles[j]) << ",";
					}
					//sum += prod;													// Used in LASTv3 with (if, if-else) statement
					//sum_d += prod_d;												// Used in LASTv3 with (if, if-else) statement
					//ofs << "," << prod << "," << prod_d << ",Pn Copy" << endl;		// Used in LASTv3 with (if, if-else) statement
					prod = 1.0;
					prod_d = 1.0;

					s_sum += s_prod;
					s_prod = 1.0;
				}
				if (population_t[i][0] == persons[ii] && is_subset_d(i)) {	// Changed in LASTv3 from if-else statement
					for (int j(0); j < population_t[i].size(); ++j) {
						if (j >= unknowns - (unknowns - s.size())) {
							prod *= population_t[i][j].freq;
						}

						if (j >= unknowns - (unknowns - d.size()))
							prod_d *= population_t[i][j].freq;

						//ofs << population_t[i][j].freq << ",";
					}
					for (int j(0); j < population_t[i].size(); ++j) {
						for (int k(0); k < rep_alleles.size(); ++k) {
							prod *= drop_out(population_t[i][j], rep_alleles[k]);
							prod_d *= drop_out(population_t[i][j], rep_alleles[k]);

							//ofs << drop_out(population_t[i][j], rep_alleles[k]) << ",";
						}
					}
					for (int j(0); j < rep_alleles.size(); ++j) {
						prod *= drop_in(population_t[i], rep_alleles[j]);
						prod_d *= drop_in(population_t[i], rep_alleles[j]);

						//ofs << drop_in(population_t[i], rep_alleles[j]) << ",";
					}
					sum += prod;
					sum_d += prod_d;
					//ofs << "," << prod << "," << prod_d << endl;
					prod = 1.0;
					prod_d = 1.0;
				}
			}
			cout << ".";
			numerators.push_back(sum);
			sum = 0.0;
		}
		cout << endl;
		numerator_sum = s_sum;
		denominator_sum = sum_d;
		for (int i(0); i < numerators.size(); ++i) {
			LRs.push_back(numerators[i] / denominator_sum);
		}

		//ofs << endl;
		//ofs.close();
	}

	double drop_out(Person p, vector<Allele>& v) {		// Determining drop-out rate for a specific genotype (Person p)
		if (p.hom) {
			for (int i(0); i < v.size(); ++i) {
				if (p.a == v[i] && p.b == v[i])
					return PHOM0;
			}
			return PHOM1;
		}
		else if (p.het) {
			bool present_first = false;
			bool present_second = false;
			for (int i(0); i < v.size(); ++i) {
				if (p.a == v[i])
					present_first = true;
				if (p.b == v[i])
					present_second = true;
			}
			if (present_first && present_second)
				return PHET0;
			else if (!present_first && !present_second)
				return PHET2;
			else if ((present_first && !present_second) || (!present_first && present_second))
				return PHET1;
		}
	}

	double drop_in(vector<Person>& p, vector<Allele>& a) {		// Determining drop-in rates for a specific replicate
		vector<Allele> tmp;
		for (int i(0); i < p.size(); ++i) {
			tmp.push_back(p[i].a);
			tmp.push_back(p[i].b);
		}
		tmp.erase(unique(begin(tmp), end(tmp)), end(tmp));
		int c(0);

		for (int i(0); i < a.size(); ++i)
			if (find(tmp.begin(), tmp.end(), a[i]) == tmp.end())
				++c;

		if (c == 0)
			return PC0;
		else if (c == 1)
			return PC1;
		else
			return PC2;
	}

	void transpose(vector<vector<Person>>& a, vector<vector<Person>>& b) {
		for (int i = 0; i < a.size(); ++i) {
			for (int j = 0; j < a[i].size(); ++j) {
				b[j].push_back(a[i][j]);
			}
			vector<Person>().swap(a[i]);
		}
		vector<vector<Person>>().swap(a);
	}

	void generate_persons() {		// Generates all possible genotypes of people that can be found based on observed alleles
		for (int i(0); i < g.alleles.size(); ++i) {
			for (int j(i); j < g.alleles.size(); ++j) {
				persons.push_back(Person("Unknown", g.alleles[i], g.alleles[j]));
			}
		}
	}

	void check_person() {		// Checks if knowns' alleles in the numerator have been observed, and acts accordingly
		for (int ii(0); ii < s.size(); ++ii) {
			bool found_two = false;
			bool found_a = false;
			bool found_b = false;
			for (int i(0); i < rep_alleles.size(); ++i) {
				for (int j(0); j < rep_alleles[i].size(); ++j) {
					if (s[ii].a == rep_alleles[i][j]) {
						found_a = true;
					}
					if (s[ii].b == rep_alleles[i][j]) {
						found_b = true;
					}
				}
			}
			if (found_a && !found_b) {		// Change second allele to wild if it is not observed
				s[ii].b = persons[persons.size() - 1].b;
				s[ii].generate_freq();
			}
			if (!found_a && found_b) {		// Change first allele to wild if it is not found
				s[ii].a = s[ii].b;
				s[ii].b = persons[persons.size() - 1].b;
				s[ii].generate_freq();
			}
			if (!found_a && !found_b) {		// Chane both alleles to wild if it is not found
				s[ii].b = persons[persons.size() - 1].b;
				s[ii].a = persons[persons.size() - 1].a;
				s[ii].generate_freq();
			}
			if (s[ii].a == s[ii].b) {		// Book keeping
				s[ii].hom = true;
				s[ii].het = false;
			}
			else {
				s[ii].het = true;
				s[ii].hom = false;
			}
		}
		for (int ii(0); ii < d.size(); ++ii) {	// For Knowns in the denominator
			bool found_two = false;
			bool found_a = false;
			bool found_b = false;
			for (int i(0); i < rep_alleles.size(); ++i) {
				for (int j(0); j < rep_alleles[i].size(); ++j) {
					if (d[ii].a == rep_alleles[i][j]) {
						found_a = true;
					}
					if (d[ii].b == rep_alleles[i][j]) {
						found_b = true;
					}
				}
			}
			if (found_a && !found_b) {		// Change second allele to wild if it is not observed
				d[ii].b = persons[persons.size() - 1].b;
				d[ii].generate_freq();
			}
			if (!found_a && found_b) {		// Change first allele to wild if it is not found
				d[ii].a = d[ii].b;
				d[ii].b = persons[persons.size() - 1].b;
				d[ii].generate_freq();
			}
			if (!found_a && !found_b) {		// Chane both alleles to wild if it is not found
				d[ii].b = persons[persons.size() - 1].b;
				d[ii].a = persons[persons.size() - 1].a;
				d[ii].generate_freq();
			}
			if (d[ii].a == d[ii].b) {		// Book keeping
				d[ii].hom = true;
				d[ii].het = false;
			}
			else {
				d[ii].het = true;
				d[ii].hom = false;
			}
		}
	}

	// GETTERS and SETTERS

	vector<Person> get_suspect() const { return s; }

	vector<Person> get_known_pd() const { return d; }

	int get_unknowns() const { return unknowns; }

	double get_LR() const { return LR; }

	vector<Person>& get_persons() { return persons; }

	vector<double>& get_LRs() { return LRs; }

private:
	double numerator_sum;
	double denominator_sum;
	double LR;

	vector<double> LRs;
	vector<double> numerators;
	vector<Person> persons;
	vector<vector<Allele>> rep_alleles;
	vector<vector<Person>> population;		// Population accounted for in the demominator
	vector<vector<Person>> population_t;	// Transposed population
	vector<vector<int>> indicies;
	vector<Person> s;						// Numberator (Pn) Knowns
	vector<Person> d;						// Denominator (Pd) Knowns

	Genotype_Combination g;
	int rep_num;
	int unknowns;							// Number of contributors in the numerator/deniminator - ideally the number of knowns cannot exceed this number
	bool flag;								// Not very important

	friend Allele;
	friend Genotype_Combination;
	friend Person;
};

// Parameters: vector<Allele>& alleles
// Returns double
double generate_wild_allele_freq(vector<Allele>& alleles) {
	double c(1.0);
	for (int i(0); i < alleles.size(); ++i) {
		c -= alleles[i].get_freq();
	}
	if (c < 0)
		c = MINIMUM_WILD_FREQUENCY;
	return c;
}

// Parameters: csv_database& db, string& locus, double length, char race
// Returns double
double get_input_freq(csv_database& db, string& locus, double length, char race) {
	for (int i(0); i < db.size(); ++i) {
		for (int j(0); j < db[i].size(); ++j) {
			if (db[i][j] == locus && atof(db[i][j + 1].c_str()) == length && race == 'b')
				return atof(db[i][j + 2].c_str());
			else if (db[i][j] == locus && atof(db[i][j + 1].c_str()) == length && race == 'c')
				return atof(db[i][j + 3].c_str());
			else if (db[i][j] == locus && atof(db[i][j + 1].c_str()) == length && race == 'h')
				return atof(db[i][j + 4].c_str());
			else if (db[i][j] == locus && atof(db[i][j + 1].c_str()) == length && race == 'a')
				return atof(db[i][j + 5].c_str());
		}
	}
	cout << "Frequency not found - LR given will most likely be wrong!" << endl;
	return 0.0;
}

// Paramters: csv_database& db, string locus, string length, char race
// Returns string
string get_input_freq_string(csv_database& db, string locus, string length, char race) {		// Not used
	for (int i(0); i < db.size(); ++i) {
		for (int j(0); j < db[i].size(); ++j) {
			if (db[i][j] == locus && db[i][j + 1] == length && race == 'b')
				return db[i][j + 2];
			else if (db[i][j] == locus && db[i][j + 1] == length && race == 'c')
				return db[i][j + 3];
			else if (db[i][j] == locus && db[i][j + 1] == length && race == 'h')
				return db[i][j + 4];
			else if (db[i][j] == locus && db[i][j + 1] == length && race == 'a')
				return db[i][j + 5];
		}
	}
	string str = "0.0";
	return str;
}

// Parameters: csv_database& db, string db_name
// Returns double (0.0 or 1.0)
double read_csv(csv_database& db, string db_name) {
	ifstream ifs(db_name);
	String csvLine;
	while (getline(ifs, csvLine)) {
		istringstream csvStream(csvLine);
		csv_row csvRow;
		String csvCol;
		while (getline(csvStream, csvCol, ','))
			csvRow.push_back(csvCol);
		db.push_back(csvRow);
	}
	ifs.close();
	return true;
}

// Parameters: csv_database& db, string db_name, int rows
// Returns double (0.0 or 1.0)
double read_csv_x_rows(csv_database& db, string db_name, int rows) {		// Reads x number of rows rows only
	ifstream ifs(db_name);
	String csvLine;
	int i(1);
	while (getline(ifs, csvLine)) {
		istringstream csvStream(csvLine);
		csv_row csvRow;
		String csvCol;
		while (getline(csvStream, csvCol, ','))
			csvRow.push_back(csvCol);
		db.push_back(csvRow);
		if (i == rows) break;
		++i;
	}
	ifs.close();
	return true;
}

// Parameters: csv_database& db, csv_database& adb, vector<vector<Allele>>& a, vector<Allele>& b, int& c, double& q, char race
// Returns vector<Person>
vector<vector<Person>> get_data(csv_database& db, csv_database& adb, vector<vector<Allele>>& a, vector<Allele>& b, int& c, double& q, char race) {
	string locus;
	string name("Known");
	vector<Allele> s_tmp;	// Numerator (Pn) Knowns
	vector<Allele> p_tmp;	// Denominator (Pd) Knowns
	vector<Allele> r_tmp;	// Vector for a replicate
	for (int i(0); i < db[0].size(); ++i) {
		if (db[0][i] == "Case Name")
			name = db[1][i];
		else if (db[0][i] == "Locus")
			locus = db[1][i];
		else if (db[0][i] == "Contributors")
			c = atof(db[1][i].c_str());
		else if (db[0][i] == "Quant")
			q = atof(db[1][i].c_str());
		else if (db[0][i] == "Alleles") {
			istringstream csvCell(db[1][i]);
			string sub_cell;
			while (getline(csvCell, sub_cell, ';')) {
				Allele tmp_a(locus, atof(sub_cell.c_str()), get_input_freq(adb, locus, atof(sub_cell.c_str()), race), true);
				if (tmp_a.get_length() != -1) {
					b.push_back(tmp_a);
				}
			}
		}
		else if (db[0][i] == "Known Pn") {
			istringstream csvCell(db[1][i]);
			string sub_cell;
			while (getline(csvCell, sub_cell, ';')) {
				Allele tmp_a(locus, atof(sub_cell.c_str()), get_input_freq(adb, locus, atof(sub_cell.c_str()), race), true);
				if (tmp_a.get_length() != -1) {
					s_tmp.push_back(tmp_a);
				}
			}
		}
		else if (db[0][i] == "Known Pd") {
			istringstream csvCell(db[1][i]);
			string sub_cell;
			while (getline(csvCell, sub_cell, ';')) {
				Allele tmp_a(locus, atof(sub_cell.c_str()), get_input_freq(adb, locus, atof(sub_cell.c_str()), race), true);
				if (tmp_a.get_length() != -1) {
					p_tmp.push_back(tmp_a);
				}
			}
		}
		else if (db[0][i] == "D/ND") {
			if (db[1][i] == "d" || db[1][i] == "D" || db[1][i] == "yes") {
				DEDUCIBLE = true;
			}
			else if (db[1][i] == "nd" || db[1][i] == "ND" || db[1][i] == "no") {
				DEDUCIBLE = false;
			}
			else {
				cout << "Error(s) in the D/ND column or another column!" << endl;
			}
		}
		else if (db[0][i] == "REP") {
			istringstream csvCell(db[1][i]);
			string sub_cell;
			while (getline(csvCell, sub_cell, ';')) {
				Allele tmp_a(locus, atof(sub_cell.c_str()), get_input_freq(adb, locus, atof(sub_cell.c_str()), race), true);
				if (tmp_a.get_length() != -1) {
					r_tmp.push_back(tmp_a);
				}
			}
			a.push_back(r_tmp);
			r_tmp.clear();
		}
	}
	double freq = generate_wild_allele_freq(b);
	Allele wild(locus + "-W", -1, freq, true);
	b.push_back(wild);

	vector<Person> s;		// Vector of Numerator (Pn) Knowns
	vector<Person> p;		// Vector of Denominator (Pd) Knowns
	for (int i(0); i < s_tmp.size(); i += 2) {
		Person known(name + " " + to_string(i / 2 + 1), s_tmp[i], s_tmp[i + 1]);
		s.push_back(known);
	}
	for (int i(0); i < p_tmp.size(); i += 2) {
		Person known(name + " " + to_string(i / 2 + 1), p_tmp[i], p_tmp[i + 1]);
		p.push_back(known);
	}
	vector<vector<Person>> ret;
	ret.push_back(s);
	ret.push_back(p);
	return ret;
}

// Parameters: vector<vector<string>>& allele_database, vector<vector<string>>& dordb, int e, char race
// Returns double
double run_LAST(vector<vector<string>>& allele_database, vector<vector<string>>& dordb, int e, char race) {
	timer time;
	double nuClicks;

	vector<vector<string>> evidence_database;
	read_csv_x_rows(evidence_database, "Evidence_" + to_string(e) + ".csv", 2);

	vector<vector<Allele>> replicates;
	vector<Allele> alleles;
	int contributors(2);
	double quant(100.0);
	string dnd("ND");
	vector<vector<Person>> knowns = get_data(evidence_database, allele_database, replicates, alleles, contributors, quant, race);

	double low_copy_pC0(0.0), low_copy_pC1(0.0), low_copy_pC2(0.0), high_copy_pC0(0.0), high_copy_pC1(0.0), high_copy_pC2(0.0), theta(0.0), min_w(0.0);
	vector<vector<string>> drop_in_database;
	vector<string> drop_in_col(7);
	drop_in_database.push_back(drop_in_col);
	drop_in_database.push_back(drop_in_col);
	read_csv(drop_in_database, "Drop_In_Rates.csv");
	for (int i(0); i < drop_in_database.size(); ++i) {
		for (int j(0); j < drop_in_database[i].size(); ++j) {
			if (drop_in_database[i][j] == "LPC0")
				low_copy_pC0 = atof(drop_in_database[i][j + 1].c_str());
			else if (drop_in_database[i][j] == "LPC1")
				low_copy_pC1 = atof(drop_in_database[i][j + 1].c_str());
			else if (drop_in_database[i][j] == "LPC2")
				low_copy_pC2 = atof(drop_in_database[i][j + 1].c_str());
			else if (drop_in_database[i][j] == "HPC0")
				high_copy_pC0 = atof(drop_in_database[i][j + 1].c_str());
			else if (drop_in_database[i][j] == "HPC1")
				high_copy_pC1 = atof(drop_in_database[i][j + 1].c_str());
			else if (drop_in_database[i][j] == "HPC2")
				high_copy_pC2 = atof(drop_in_database[i][j + 1].c_str());
			else if (drop_in_database[i][j] == "THETA")
				theta = atof(drop_in_database[i][j + 1].c_str());
			else if (drop_in_database[i][j] == "B-MIN-WILD-FREQ" && race == 'b')
				min_w = atof(drop_in_database[i][j + 1].c_str());
			else if (drop_in_database[i][j] == "C-MIN-WILD-FREQ" && race == 'c')
				min_w = atof(drop_in_database[i][j + 1].c_str());
			else if (drop_in_database[i][j] == "H-MIN-WILD-FREQ" && race == 'h')
				min_w = atof(drop_in_database[i][j + 1].c_str());
			else if (drop_in_database[i][j] == "A-MIN-WILD-FREQ" && race == 'a')
				min_w = atof(drop_in_database[i][j + 1].c_str());
		}
	}
	HOM_CONST = theta;
	MINIMUM_WILD_FREQUENCY = min_w;		// Setting minimum wild frequency in case "w" becomes negative
	if (quant <= 100) {					// QUANT of 100 will be run as low copy
		PC0 = low_copy_pC0;
		PC1 = low_copy_pC1;
		PC2 = low_copy_pC2;
	}
	else {
		PC0 = high_copy_pC0;
		PC1 = high_copy_pC1;
		PC2 = high_copy_pC2;
	}

	if (quant < 6.25 && contributors == 1) {
		quant = 6.25;
	}
	else if (quant < 25.0 && contributors > 1) {
		quant = 25.0;
	}
	else if (quant > 500) {
		quant = 500;
	}

	QUANT = quant;

	if (DEDUCIBLE) dnd = "D";
	else dnd = "ND";

	double full(1.0);
	for (int i(0); i < dordb.size(); ++i) {					// Getting proper drop-out rates based on quant and number of contributors
		for (int j(0); j < dordb[i].size(); ++j) {
			if (dordb[i][j] == "HOM1-" + to_string(contributors) + "-" + dnd && dordb[i][j + 1] == knowns[0][0].get_a().get_locus() && quant >= 25 && quant < 50) {
				double b(0.0), l(25.0), h(50.0);
				double slope((atof(dordb[i][j + 3].c_str()) - atof(dordb[i][j + 2].c_str())) / (h - l));
				b = atof(dordb[i][j + 2].c_str()) - slope * l;
				PHOM1 = slope * quant + b;
				PHOM0 = full - PHOM1;

			}
			else if (dordb[i][j] == "HOM1-" + to_string(contributors) + "-" + dnd && dordb[i][j + 1] == knowns[0][0].get_a().get_locus() && quant >= 50 && quant < 100) {
				double b(0.0), l(50.0), h(100.0);
				double slope((atof(dordb[i][j + 4].c_str()) - atof(dordb[i][j + 3].c_str())) / (h - l));
				b = atof(dordb[i][j + 3].c_str()) - slope * l;
				PHOM1 = slope * quant + b;
				PHOM0 = full - PHOM1;
			}
			else if (dordb[i][j] == "HOM1-" + to_string(contributors) + "-" + dnd && dordb[i][j + 1] == knowns[0][0].get_a().get_locus() && quant >= 100 && quant < 150) {
				double b(0.0), l(100.0), h(150.0);
				double slope((atof(dordb[i][j + 5].c_str()) - atof(dordb[i][j + 4].c_str())) / (h - l));
				b = atof(dordb[i][j + 4].c_str()) - slope * l;
				PHOM1 = slope * quant + b;
				PHOM0 = full - PHOM1;
			}
			else if (dordb[i][j] == "HOM1-" + to_string(contributors) + "-" + dnd && dordb[i][j + 1] == knowns[0][0].get_a().get_locus() && quant >= 150 && quant < 250) {
				double b(0.0), l(150.0), h(250.0);
				double slope((atof(dordb[i][j + 6].c_str()) - atof(dordb[i][j + 5].c_str())) / (h - l));
				b = atof(dordb[i][j + 5].c_str()) - slope * l;
				PHOM1 = slope * quant + b;
				PHOM0 = full - PHOM1;
			}
			else if (dordb[i][j] == "HOM1-" + to_string(contributors) + "-" + dnd && dordb[i][j + 1] == knowns[0][0].get_a().get_locus() && quant >= 250 && quant <= 500) {
				double b(0.0), l(250.0), h(500.0);
				double slope((atof(dordb[i][j + 7].c_str()) - atof(dordb[i][j + 6].c_str())) / (h - l));
				b = atof(dordb[i][j + 6].c_str()) - slope * l;
				PHOM1 = slope * quant + b;
				PHOM0 = full - PHOM1;
			}
			else if (dordb[i][j] == "HOM1-" + to_string(contributors) + "-" + dnd && dordb[i][j + 1] == knowns[0][0].get_a().get_locus() && quant >= 6.25 && quant < 12.5) { // Single Source
				double b(0.0), l(6.25), h(12.5);
				double slope((atof(dordb[i][j + 9].c_str()) - atof(dordb[i][j + 8].c_str())) / (h - l));
				b = atof(dordb[i][j + 8].c_str()) - slope * l;
				PHOM1 = slope * quant + b;
				PHOM0 = full - PHOM1;
			}
			else if (dordb[i][j] == "HOM1-" + to_string(contributors) + "-" + dnd && dordb[i][j + 1] == knowns[0][0].get_a().get_locus() && quant >= 12.5 && quant < 25) {
				double b(0.0), l(12.5), h(25);
				double slope((atof(dordb[i][j + 2].c_str()) - atof(dordb[i][j + 9].c_str())) / (h - l));
				b = atof(dordb[i][j + 9].c_str()) - slope * l;
				PHOM1 = slope * quant + b;
				PHOM0 = full - PHOM1;
			}

			else if (dordb[i][j] == "HET1-" + to_string(contributors) + "-" + dnd && dordb[i][j + 1] == knowns[0][0].get_a().get_locus() && quant >= 25 && quant < 50) {
				double b(0.0), l(25.0), h(50.0);
				double slope((atof(dordb[i][j + 3].c_str()) - atof(dordb[i][j + 2].c_str())) / (h - l));
				b = atof(dordb[i][j + 2].c_str()) - slope * l;
				PHET1 = slope * quant + b;
			}
			else if (dordb[i][j] == "HET1-" + to_string(contributors) + "-" + dnd && dordb[i][j + 1] == knowns[0][0].get_a().get_locus() && quant >= 50 && quant < 100) {
				double b(0.0), l(50.0), h(100.0);
				double slope((atof(dordb[i][j + 4].c_str()) - atof(dordb[i][j + 3].c_str())) / (h - l));
				b = atof(dordb[i][j + 3].c_str()) - slope * l;
				PHET1 = slope * quant + b;
			}
			else if (dordb[i][j] == "HET1-" + to_string(contributors) + "-" + dnd && dordb[i][j + 1] == knowns[0][0].get_a().get_locus() && quant >= 100 && quant < 150) {
				double b(0.0), l(100.0), h(150.0);
				double slope((atof(dordb[i][j + 5].c_str()) - atof(dordb[i][j + 4].c_str())) / (h - l));
				b = atof(dordb[i][j + 4].c_str()) - slope * l;
				PHET1 = slope * quant + b;
			}
			else if (dordb[i][j] == "HET1-" + to_string(contributors) + "-" + dnd && dordb[i][j + 1] == knowns[0][0].get_a().get_locus() && quant >= 150 && quant < 250) {
				double b(0.0), l(150.0), h(250.0);
				double slope((atof(dordb[i][j + 6].c_str()) - atof(dordb[i][j + 5].c_str())) / (h - l));
				b = atof(dordb[i][j + 5].c_str()) - slope * l;
				PHET1 = slope * quant + b;
			}
			else if (dordb[i][j] == "HET1-" + to_string(contributors) + "-" + dnd && dordb[i][j + 1] == knowns[0][0].get_a().get_locus() && quant >= 250 && quant <= 500) {
				double b(0.0), l(250.0), h(500.0);
				double slope((atof(dordb[i][j + 7].c_str()) - atof(dordb[i][j + 6].c_str())) / (h - l));
				b = atof(dordb[i][j + 6].c_str()) - slope * l;
				PHET1 = slope * quant + b;
			}
			else if (dordb[i][j] == "HET1-" + to_string(contributors) + "-" + dnd && dordb[i][j + 1] == knowns[0][0].get_a().get_locus() && quant >= 6.25 && quant < 12.5) { // Single Source
				double b(0.0), l(6.25), h(12.5);
				double slope((atof(dordb[i][j + 9].c_str()) - atof(dordb[i][j + 8].c_str())) / (h - l));
				b = atof(dordb[i][j + 8].c_str()) - slope * l;
				PHET1 = slope * quant + b;
			}
			else if (dordb[i][j] == "HET1-" + to_string(contributors) + "-" + dnd && dordb[i][j + 1] == knowns[0][0].get_a().get_locus() && quant >= 12.5 && quant < 25) {
				double b(0.0), l(12.5), h(25.0);
				double slope((atof(dordb[i][j + 2].c_str()) - atof(dordb[i][j + 9].c_str())) / (h - l));
				b = atof(dordb[i][j + 9].c_str()) - slope * l;
				PHET1 = slope * quant + b;
			}

			else if (dordb[i][j] == "HET2-" + to_string(contributors) + "-" + dnd && dordb[i][j + 1] == knowns[0][0].get_a().get_locus() && quant >= 25 && quant < 50) {
				double b(0.0), l(25.0), h(50.0);
				double slope((atof(dordb[i][j + 3].c_str()) - atof(dordb[i][j + 2].c_str())) / (h - l));
				b = atof(dordb[i][j + 2].c_str()) - slope * l;
				PHET2 = slope * quant + b;
				PHET0 = full - (PHET1 + PHET2);
			}
			else if (dordb[i][j] == "HET2-" + to_string(contributors) + "-" + dnd && dordb[i][j + 1] == knowns[0][0].get_a().get_locus() && quant >= 50 && quant < 100) {
				double b(0.0), l(50.0), h(100.0);
				double slope((atof(dordb[i][j + 4].c_str()) - atof(dordb[i][j + 3].c_str())) / (h - l));
				b = atof(dordb[i][j + 3].c_str()) - slope * l;
				PHET2 = slope * quant + b;
				PHET0 = full - (PHET1 + PHET2);
			}
			else if (dordb[i][j] == "HET2-" + to_string(contributors) + "-" + dnd && dordb[i][j + 1] == knowns[0][0].get_a().get_locus() && quant >= 100 && quant < 150) {
				double b(0.0), l(100.0), h(150.0);
				double slope((atof(dordb[i][j + 5].c_str()) - atof(dordb[i][j + 4].c_str())) / (h - l));
				b = atof(dordb[i][j + 4].c_str()) - slope * l;
				PHET2 = slope * quant + b;
				PHET0 = full - (PHET1 + PHET2);
			}
			else if (dordb[i][j] == "HET2-" + to_string(contributors) + "-" + dnd && dordb[i][j + 1] == knowns[0][0].get_a().get_locus() && quant >= 150 && quant < 250) {
				double b(0.0), l(150.0), h(250.0);
				double slope((atof(dordb[i][j + 6].c_str()) - atof(dordb[i][j + 5].c_str())) / (h - l));
				b = atof(dordb[i][j + 5].c_str()) - slope * l;
				PHET2 = slope * quant + b;
				PHET0 = full - (PHET1 + PHET2);
			}
			else if (dordb[i][j] == "HET2-" + to_string(contributors) + "-" + dnd && dordb[i][j + 1] == knowns[0][0].get_a().get_locus() && quant >= 250 && quant <= 500) {
				double b(0.0), l(250.0), h(500.0);
				double slope((atof(dordb[i][j + 7].c_str()) - atof(dordb[i][j + 6].c_str())) / (h - l));
				b = atof(dordb[i][j + 6].c_str()) - slope * l;
				PHET2 = slope * quant + b;
				PHET0 = full - (PHET1 + PHET2);
			}
			else if (dordb[i][j] == "HET2-" + to_string(contributors) + "-" + dnd && dordb[i][j + 1] == knowns[0][0].get_a().get_locus() && quant >= 6.25 && quant < 12.5) { // Single Source
				double b(0.0), l(6.25), h(12.5);
				double slope((atof(dordb[i][j + 9].c_str()) - atof(dordb[i][j + 8].c_str())) / (h - l));
				b = atof(dordb[i][j + 8].c_str()) - slope * l;
				PHET2 = slope * quant + b;
				PHET0 = full - (PHET1 + PHET2);
			}
			else if (dordb[i][j] == "HET2-" + to_string(contributors) + "-" + dnd && dordb[i][j + 1] == knowns[0][0].get_a().get_locus() && quant >= 12.5 && quant < 25) {
				double b(0.0), l(12.5), h(25.0);
				double slope((atof(dordb[i][j + 2].c_str()) - atof(dordb[i][j + 9].c_str())) / (h - l));
				b = atof(dordb[i][j + 9].c_str()) - slope * l;
				PHET2 = slope * quant + b;
				PHET0 = full - (PHET1 + PHET2);
			}
		}
	}

	Genotype_Combination genotypes(alleles);
	Report report(contributors, knowns[0], knowns[1], genotypes, replicates);
	return report.get_LR();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// MAIN
//
//

Allele NULL_ALLELE("NULL", 0.0, 0.0, false);	// Not to be confused with WILD (w) Allele
												// Stops argument list - Used in previous versions of LAST for Vardic classes and functions

int main() {

	cout << "///////////////////////////////////////////////////////" << endl;
	cout << "//" << endl;
	cout << "//\tPROGRAM HAS BEEN INITIALIZED AND IS RUNNING" << endl;
	cout << "//" << endl;
	cout << "//" << endl;
	cout << "\n________________________________________________\n" << endl;
	timer time;
	ofstream ofs("output.csv");		// Overwriting file on purpose
	ofstream ofst("output.txt");	// Overwriting file on purpose
	ofs.close();
	ofst.close();

	double nuClicks(0.0), repClicks(0.0);
	double LR_b(1.0), LR_c(1.0), LR_h(1.0), LR_a(1.0);

	vector<vector<string>> allele_database(6);
	read_csv(allele_database, "Allele_Frequencies.csv");
	vector<vector<string>> drop_out_rates_database(8);
	read_csv(drop_out_rates_database, "Drop_Out_Rates.csv");
	vector<double> LRs;

	time.reset();
	while (true) {
		ifstream ifs("Evidence_" + to_string(FILE_NUM) + ".csv");
		if (ifs) {
			cout << "\nRunning Analysis for Black for Evidence " << FILE_NUM << endl;
			LR_b *= run_LAST(allele_database, drop_out_rates_database, FILE_NUM, 'b');
			++RACE;

			cout << "\nRunning Analysis for Caucasian for Evidence " << FILE_NUM << endl;
			LR_c *= run_LAST(allele_database, drop_out_rates_database, FILE_NUM, 'c');
			++RACE;

			cout << "\nRunning Analysis for Hispanic for Evidence " << FILE_NUM << endl;
			LR_h *= run_LAST(allele_database, drop_out_rates_database, FILE_NUM, 'h');
			++RACE;

			cout << "\nRunning Analysis for Asian for Evidence " << FILE_NUM << endl;
			LR_a *= run_LAST(allele_database, drop_out_rates_database, FILE_NUM, 'a');
			RACE = 1;
		}
		cout << "\n_____________________________________\n" << endl;
		if (FILE_NUM % 15 == 0) {		// Calculate likelihood ratio once 15 files (or loci) has been evaluated - this is one full case
			repClicks = time.elapsed();
			ofs.open("output.csv", ios::app);
			ofst.open("output.txt", ios::app);
			ofs << "\nOverall LR:\n,BLACK:," << LR_b << ",,CAUCASIAN:," << LR_c << ",,HISPANIC:," << LR_h << ",,ASIAN:," << LR_a << endl;
			ofst << "\nOverall LR:\n\tBLACK:\t\t" << LR_b << "\n\tCAUCASIAN:\t" << LR_c << "\n\tHISPANIC:\t" << LR_h << "\n\tASIAN:\t\t" << LR_a << endl;
			ofs << "\n,Time:," << repClicks << ",second(s)" << endl;
			ofst << "\n\tTime:\t" << repClicks << " second(s)" << endl;
			ofs.close();
			ofst.close();
			nuClicks += repClicks;
			time.reset();
			LR_b = 1.0;
			LR_c = 1.0;
			LR_h = 1.0;
			LR_a = 1.0;
		}
		++FILE_NUM;
		if (FILE_NUM == 61) {	// One can run 4 full cases at a time while resetting the overall likelihood ratios for every 15 files
			FILE_NUM = 1;
			break;
		}
	}

	cout << "\n\tTime to run:\t" << nuClicks << " second(s)" << endl;
	cout << "\n\t\tLAST Analysis Finished!\n" << endl;
	ofs.open("output.csv", ios::app);
	ofst.open("output.txt", ios::app);
	ofs << "\n,Total Time:," << nuClicks << ",second(s)" << endl;
	ofst << "\n\tTotal Time:\t" << nuClicks << " second(s)" << endl;
	ofs.close();
	ofst.close();
	system("pause");
}
