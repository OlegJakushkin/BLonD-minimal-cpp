#include <blond/impedances/InducedVoltage.h>

#include <functional>
#include <vector>
#include <algorithm>
#include <blond/globals.h>
#include <blond/constants.h>
#include <blond/math_functions.h>
#include <blond/input_parameters/GeneralParameters.h>
#include <blond/impedances/Ham.h>
#include <blond/beams/Slices.h>
#include <blond/beams/Beams.h>
using namespace blond;


std::vector<ftype> TotalInducedVoltage::induced_voltage_generation(unsigned int length)
{
   return std::vector<ftype>();
}


inline void InducedVoltage::linear_interp_kick(
   const ftype *__restrict beam_dt,
   ftype *__restrict beam_dE,
   const ftype *__restrict voltage_array,
   const ftype *__restrict bin_centers,
   const int n_slices,
   const int n_macroparticles,
   const ftype acc_kick)
{
   
   const ftype binFirst = bin_centers[0];
   const ftype binLast = bin_centers[n_slices - 1];

   const ftype inv_bin_width = (n_slices - 1) / (binLast - binFirst);
   
   #pragma omp parallel for
   for (int i = 0; i < n_macroparticles; i++) {
      ftype a = beam_dt[i];
      //ftype fbin = (a - binFirst) * inv_bin_width;
      int ffbin = static_cast<int>((a - binFirst) * inv_bin_width);
      //unsigned ffbin = (unsigned)(fbin);
      ftype voltageKick = ((a < binFirst) || (a > binLast)) ?
                          0 : voltage_array[ffbin] + (a - bin_centers[ffbin])
                          * (voltage_array[ffbin + 1] - voltage_array[ffbin])
                          * inv_bin_width;
      beam_dE[i] += voltageKick + acc_kick;
   }
   
   
   //ftype inv_bin_width = (n_slices-1) / (bin_centers[n_slices-1] - bin_centers[0]);
   /*
   double inv_bin_width = (n_slices-1) / (bin_centers[n_slices-1] - bin_centers[0]);
   #pragma omp parallel for
   for (int i = 0; i < n_macroparticles; i++) {
      double a = beam_dt[i];
      double fbin = (a - bin_centers[0]) * inv_bin_width;
      int ffbin = (int)(fbin);
      double voltageKick;
      if ((a < bin_centers[0])||(a > bin_centers[n_slices-1]))
         voltageKick = 0.;
      else
         voltageKick = voltage_array[ffbin] + (a - bin_centers[ffbin]) * (voltage_array[ffbin+1]-voltage_array[ffbin]) * inv_bin_width;
      beam_dE[i] = beam_dE[i] + voltageKick + acc_kick;
    }
   */

}

InducedVoltageTime::InducedVoltageTime(std::vector<Intensity *> &WakeSourceList,
                                       time_or_freq TimeOrFreq)
{

	auto Slice = context.Slice;
   // Induced voltage derived from the sum of
   // several wake fields (time domain).*

   // *Wake sources inputed as a list (eg: list of BBResonators objects)*
   fWakeSourceList = WakeSourceList;

   // *Time array of the wake in [s]*
   fTimeArray = std::vector<ftype>();

   // *Total wake array of all sources in* [:math:`\Omega / s`]
   fTotalWake = std::vector<ftype>();

   // *Induced voltage from the sum of the wake sources in [V]*
   fInducedVoltage = std::vector<ftype>();

   // Pre-processing the wakes
   fTimeArray.resize(Slice->n_slices);
   for (unsigned int i = 0; i < fTimeArray.size(); ++i) {
      fTimeArray[i] = Slice->bin_centers[i] - Slice->bin_centers[0];
   }
   sum_wakes(fTimeArray);

   fCut = fTimeArray.size() + Slice->n_slices - 1;
   fShape = next_regular(fCut);

   fTimeOrFreq = TimeOrFreq;

}


inline void InducedVoltageTime::track()
{
	auto GP = context.GP;
	auto Beam = context.Beam;
	auto Slice = context.Slice;
   // Tracking Method
   std::vector<ftype> v = this->induced_voltage_generation();

   //std::cout << "induced v size is " << v.size() << "\n";

   std::transform(v.begin(), v.end(), v.begin(),
                  std::bind1st(std::multiplies<ftype>(),
                               GP->charge));

   linear_interp_kick(Beam->dt, Beam->dE, v.data(),
                      Slice->bin_centers, Slice->n_slices,
                      Beam->n_macroparticles, 0.0);

}

void InducedVoltageTime::sum_wakes(std::vector<ftype> &TimeArray)
{
   // *Summing all the wake contributions in one total wake.*
   fTotalWake.resize(TimeArray.size());
   std::fill(fTotalWake.begin(), fTotalWake.end(), 0);
   for (Intensity *i : fWakeSourceList) {

      i->wake_calc(TimeArray);
      std::transform(fTotalWake.begin(), fTotalWake.end(),
                     i->fWake.begin(), fTotalWake.begin(),
                     std::plus<ftype>());

   }

}

void InducedVoltageTime::reprocess()
{
	auto Slice = context.Slice;
   // *Reprocess the wake contributions with respect to the new_slicing.*
   // WARNING As Slice is a global variable,
   // users will have to change this variable and call reprocess()
   fTimeArray.resize(Slice->n_slices);
   for (unsigned int i = 0; i < fTimeArray.size(); ++i) {
      fTimeArray[i] = Slice->bin_centers[i] - Slice->bin_centers[0];
   }
   sum_wakes(fTimeArray);

   fCut = fTimeArray.size() + Slice->n_slices - 1;
   fShape = next_regular(fCut);


}

std::vector<ftype> InducedVoltageTime::induced_voltage_generation(unsigned int length)
{
	auto GP = context.GP;
	auto Beam = context.Beam;
	auto Slice = context.Slice;
   // Method to calculate the induced voltage from wakes with convolution.*

   std::vector<ftype> inducedVoltage;

   const ftype factor = - GP->charge * constant::e *
                        Beam->intensity / Beam->n_macroparticles;

   if (fTimeOrFreq == freq_domain) {
      std::vector<complex_t> fft1, fft2;
      std::vector<ftype> in(Slice->n_macroparticles,
                            Slice->n_macroparticles + Slice->n_slices);

      mymath::real_to_complex(in, fft1);

      in.clear();
      in = fTotalWake;
      mymath::real_to_complex(in, fft2);

      mymath::fft(fft1, fShape, fft1);
      mymath::fft(fft2, fShape, fft2);

      std::transform(fft1.begin(), fft1.end(), fft2.begin(),
                     fft1.begin(), std::multiplies<complex_t>());

      mymath::ifft(fft1, fShape, fft1);

      mymath::complex_to_real(fft1, inducedVoltage);

      std::transform(inducedVoltage.begin(),
                     inducedVoltage.end(),
                     inducedVoltage.begin(),
                     std::bind1st(std::multiplies<ftype>(), factor));

   } else if (fTimeOrFreq == time_domain) {
      std::vector<ftype> temp(Slice->n_macroparticles,
                              Slice->n_macroparticles + Slice->n_slices);
      inducedVoltage =
         mymath::convolution(fTotalWake, temp);

      std::transform(inducedVoltage.begin(),
                     inducedVoltage.end(),
                     inducedVoltage.begin(),
                     std::bind1st(std::multiplies<ftype>(), factor));

   } else {
      dprintf("Error: Only freq_domain or time_domain are allowed\n");
      exit(-1);
   }

   inducedVoltage.resize((unsigned) Slice->n_slices);
   fInducedVoltage = inducedVoltage;

   if (length > 0)
      inducedVoltage.resize(
         std::min((unsigned) Slice->n_slices, length), 0);

   //std::cout << "inducedVoltage size is " << inducedVoltage.size() << "\n";
   return inducedVoltage;

}


InducedVoltageFreq::InducedVoltageFreq() {}

void InducedVoltageFreq::track() {}

void InducedVoltageFreq::sum_impedances() {}

void InducedVoltageFreq::reprocess() {}

std::vector<ftype> InducedVoltageFreq::induced_voltage_generation(unsigned int length)
{
   return std::vector<ftype>();
}


TotalInducedVoltage::TotalInducedVoltage(
   std::vector<InducedVoltage *> &InducedVoltageList,
   unsigned int NTurnsMemory,
   std::vector<ftype> RevTimeArray)
{
	auto Slice = context.Slice;
   fInducedVoltageList = InducedVoltageList;
   fNTurnsMemory = NTurnsMemory;
   fInducedVoltage = std::vector<ftype>();
   fTimeArray = std::vector<ftype> (Slice->bin_centers,
                                    Slice->bin_centers + Slice->n_slices);

}

void TotalInducedVoltage::track()
{
	auto GP = context.GP;
	auto Beam = context.Beam;
	auto Slice = context.Slice;
   //std::cout << "I am here\n";
   this->induced_voltage_sum();
   auto v = this->fInducedVoltage;
   //std::cout << "total v size is " << v.size() << "\n";

   std::transform(v.begin(), v.end(), v.begin(),
                  std::bind1st(std::multiplies<ftype>(),
                               GP->charge));
   //std::cout << "I am here\n";

   linear_interp_kick(Beam->dt, Beam->dE, v.data(),
                      Slice->bin_centers, Slice->n_slices,
                      Beam->n_macroparticles, 0.0);
   //std::cout << "I am here\n";

}

void TotalInducedVoltage::track_memory() {}

void TotalInducedVoltage::track_ghosts_particles() {}

void TotalInducedVoltage::reprocess()
{
   for (auto &v : fInducedVoltageList)
      v->reprocess();
}

std::vector<ftype> TotalInducedVoltage::induced_voltage_sum(unsigned int length)
{
   // Method to sum all the induced voltages in one single array.
   std::vector<ftype> tempIndVolt;
   std::vector<ftype> extIndVolt;

   for (auto &v : fInducedVoltageList) {
      std::vector<ftype> a;
      a = v->induced_voltage_generation(length);
      //std::cout << "a size is " << a.size() << '\n';
      if (length > 0) {
         extIndVolt.resize(a.size(), 0);
         std::transform(extIndVolt.begin(), extIndVolt.end(),
                        a.begin(), extIndVolt.begin(),
                        std::plus<ftype>());
      }
      tempIndVolt.resize(v->fInducedVoltage.size(), 0);
      std::transform(tempIndVolt.begin(), tempIndVolt.end(),
                     v->fInducedVoltage.begin(), tempIndVolt.begin(),
                     std::plus<ftype>());
   }

   fInducedVoltage = tempIndVolt;
   //std::cout << "extIndVolt size = " << extIndVolt.size() << std::endl;
   return extIndVolt;


}