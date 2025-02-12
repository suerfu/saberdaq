#include <iostream>
#include <cmath>
#include <vector>

using namespace std;

int main( int argc, char* argv[] ){
    vector<double> spe;
        // single photoelectron
    vector<double> spe_er;
        // error on spe
    vector<double> phpk;
        // photopeak
    vector<double> phpk_er;

    double energy(0), ly(0), ly_er(0);

    if( argc==1 ){
        double temp;
        for( int i=0; i<1024; i++){
            cout << "\nch"<<i << " spe: ";
            cin >> temp;
            spe.push_back(temp);

            cout << "ch"<<i << " spe error: ";
            cin >> temp;
            spe_er.push_back(temp);

            cout << "ch"<<i << " photopeak: ";
            cin >> temp;
            phpk.push_back(temp);

            cout << "ch"<<i << " photopeak error: ";
            cin >> temp;
            phpk_er.push_back(temp);

            cout << "add channel? (y/n): ";
            char c;
            cin >> c;
            if( c!='y' && c!='Y' )
                break;
        }
        cout << "\nenergy of the peak (keV): ";
        cin >> energy;
    }
    
    for( unsigned int i=0; i<spe.size(); i++){
        cout << "N p.e. in ch" << i << ": " << phpk[i]/spe[i] <<" +/- " << phpk[i]/spe[i] * sqrt((phpk_er[i]/phpk[i])*(phpk_er[i]/phpk[i]) + (spe_er[i]/spe[i])*(spe_er[i]/spe[i])) << endl;
        ly += phpk[i]/spe[i];
        ly_er += phpk[i]/spe[i]*phpk[i]/spe[i] * (phpk_er[i]/phpk[i])*(phpk_er[i]/phpk[i]) + (spe_er[i]/spe[i])*(spe_er[i]/spe[i]);
            // accumulate error on light yield.
            // relative errors in computing p.e. seen in each channel add in quadrature
            // and the errors on the p.e. of each channel add in quadrature.
    }
    cout << "\nN all: " << ly << " +/- " << ly_er << endl;
    cout << "\nLY: " << ly/energy << " +/- " << ly_er/energy << "p.e./keV"<< endl;
}
