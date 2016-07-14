#include "globals.h"
#include "utilities.h"
#include "math_functions.h"
#include <stdio.h>
#include <../beams/Distributions.h>
#include "../llrf/LHCNoiseFB.h"
#include <gtest/gtest.h>
#include "../trackers/Tracker.h"

// Simulation parameters --------------------------------------------------------

// Bunch parameters
const long int N_b = (long int) 1e9;            // Intensity
const ftype tau_0 = 0.4e-9;                     // Initial bunch length, 4 sigma [s]

// Machine and RF parameters
const ftype C = 26658.883;                      // Machine circumference [m]
const ftype p_i = 450e9;                        // Synchronous momentum [eV/c]
const long h = 35640;                           // Harmonic number
const ftype V = 6e6;                            // RF voltage [V]
const ftype dphi = 0;                           // Phase modulation/offset
const ftype gamma_t = 55.759505;                // Transition gamma
const ftype alpha = 1.0 / gamma_t / gamma_t;    // First order mom. comp. factor
const int alpha_order = 1;
const int n_sections = 1;
// Tracking details

unsigned N_t = 1000;          // Number of turns to track
unsigned N_p = 10001;         // Macro-particles

int n_threads = 1;
unsigned N_slices = 100;   // = (2^8)

GeneralParameters *GP;
Beams *Beam;
Slices *Slice;
RfParameters *RfP;

class testLHCNoiseFB : public ::testing::Test {

protected:

   virtual void SetUp()
   {
      f_vector_t momentum(N_t + 1);
      mymath::linspace(momentum.data(), p_i, 1.01 * p_i, N_t + 1);

      ftype *alpha_array = new ftype[(alpha_order + 1) * n_sections];

      std::fill_n(alpha_array, (alpha_order + 1) * n_sections, alpha);

      ftype *C_array = new ftype[n_sections];
      std::fill_n(C_array, n_sections, C);

      ftype *h_array = new ftype[n_sections * (N_t + 1)];
      std::fill_n(h_array, (N_t + 1) * n_sections, h);

      ftype *V_array = new ftype[n_sections * (N_t + 1)];
      std::fill_n(V_array, (N_t + 1) * n_sections, V);

      ftype *dphi_array = new ftype[n_sections * (N_t + 1)];
      std::fill_n(dphi_array, (N_t + 1) * n_sections, dphi);

      GP = new GeneralParameters(N_t, C_array, alpha_array, alpha_order, momentum.data(),
                                 proton);

      Beam = new Beams(N_p, N_b);

      RfP = new RfParameters(n_sections, h_array, V_array, dphi_array);
      longitudinal_bigaussian(tau_0 / 4, 0, 1, false);
      Slice = new Slices(N_slices);

   }


   virtual void TearDown()
   {
      // Code here will be called immediately after each test
      // (right before the destructor).
      delete GP;
      delete Beam;
      delete RfP;
      delete Slice;
   }

};



TEST_F(testLHCNoiseFB, constructor1)
{

   auto lhcnfb = new LHCNoiseFB(1.0);

   auto params = std::string("../unit-tests/references/")
                 + "LHCNoiseFB/constructor/test1/";
   f_vector_t v;

   util::read_vector_from_file(v, params + "g.txt");
   //util::dump(lhcnfb->fG, "fG\n");
   ASSERT_EQ(v.size(), lhcnfb->fG.size());

   auto epsilon = 1e-8;
   for (unsigned int i = 0; i < v.size(); ++i) {
      auto ref = v[i];
      auto real = lhcnfb->fG[i];

      ASSERT_NEAR(ref, real, epsilon * std::max(fabs(ref), fabs(real)))
            << "Testing of fG failed on i "
            << i << std::endl;
   }

   delete lhcnfb;
}


TEST_F(testLHCNoiseFB, constructor2)
{

   auto lhcnfb = new LHCNoiseFB(1.0, 0.1, 0.9, 100, false);

   auto params = std::string("../unit-tests/references/")
                 + "LHCNoiseFB/constructor/test2/";
   f_vector_t v;

   util::read_vector_from_file(v, params + "g.txt");
   //util::dump(lhcnfb->fG, "fG\n");
   ASSERT_EQ(v.size(), lhcnfb->fG.size());

   auto epsilon = 1e-8;
   for (unsigned int i = 0; i < v.size(); ++i) {
      auto ref = v[i];
      auto real = lhcnfb->fG[i];

      ASSERT_NEAR(ref, real, epsilon * std::max(fabs(ref), fabs(real)))
            << "Testing of fG failed on i "
            << i << std::endl;
   }

   delete lhcnfb;
}



TEST_F(testLHCNoiseFB, constructor3)
{

   f_vector_t a = {1, 2, 3};
   auto lhcnfb = new LHCNoiseFB(1.0, 0.1, 0.9, 100, false, a);

   auto params = std::string("../unit-tests/references/")
                 + "LHCNoiseFB/constructor/test3/";
   f_vector_t v;

   util::read_vector_from_file(v, params + "bl_meas_bbb.txt");
   //util::dump(lhcnfb->fG, "fG\n");
   ASSERT_EQ(v.size(), lhcnfb->fBlMeasBBB.size());

   delete lhcnfb;
}


TEST_F(testLHCNoiseFB, fwhm_interpolation1)
{

   auto lhcnfb = new LHCNoiseFB(1.0);
   for (int i = 0; i < Slice->n_slices; i++) {
      Slice->n_macroparticles[i] = 100 * i;
      Slice->bin_centers[i] = 1e10 * (i + 1) / Slice->n_slices;
   }

   auto params = std::string("../unit-tests/references/")
                 + "LHCNoiseFB/fwhm_interpolation/test1/";
   f_vector_t v;

   util::read_vector_from_file(v, params + "return.txt");
   //util::dump(lhcnfb->fG, "fG\n");
   auto index = mymath::arange<int>(10, 20);
   auto epsilon = 1e-8;
   auto ref = v[0];
   auto real = lhcnfb->fwhm_interpolation(index, 1e-9);
   ASSERT_NEAR(ref, real, epsilon * std::max(fabs(ref), fabs(real)));


   delete lhcnfb;
}


TEST_F(testLHCNoiseFB, fwhm_interpolation2)
{

   auto lhcnfb = new LHCNoiseFB(1.0);
   for (int i = 0; i < Slice->n_slices; i++) {
      Slice->n_macroparticles[i] = 100 * i;
      Slice->bin_centers[i] = 1e10 * (i + 1) / Slice->n_slices;
   }

   auto params = std::string("../unit-tests/references/")
                 + "LHCNoiseFB/fwhm_interpolation/test2/";
   f_vector_t v;

   util::read_vector_from_file(v, params + "return.txt");
   //util::dump(lhcnfb->fG, "fG\n");
   auto index = mymath::arange<int>(0, 99);
   auto epsilon = 1e-8;
   auto ref = v[0];
   auto real = lhcnfb->fwhm_interpolation(index, 1);
   ASSERT_NEAR(ref, real, epsilon * std::max(fabs(ref), fabs(real)));


   delete lhcnfb;
}


TEST_F(testLHCNoiseFB, fwhm_single_bunch1)
{

   auto lhcnfb = new LHCNoiseFB(1.0);
   for (int i = 0; i < Slice->n_slices; i++) {
      Slice->n_macroparticles[i] = (Slice->n_slices - i);
      Slice->bin_centers[i] = 1e10 * (i + 1) / Slice->n_slices;
   }
   lhcnfb->fwhm_single_bunch();

   auto params = std::string("../unit-tests/references/")
                 + "LHCNoiseFB/fwhm_single_bunch/test1/";
   f_vector_t v;

   util::read_vector_from_file(v, params + "bl_meas.txt");
   //util::dump(lhcnfb->fG, "fG\n");
   auto epsilon = 1e-8;
   auto ref = v[0];
   auto real = lhcnfb->fBlMeas;
   ASSERT_NEAR(ref, real, epsilon * std::max(fabs(ref), fabs(real)));


   delete lhcnfb;
}


TEST_F(testLHCNoiseFB, DISABLED_fwhm_multi_bunch1)
{

   f_vector_t a = {1, 2, 3, 4, 5};
   auto lhcnfb = new LHCNoiseFB(1.0, 0.1, 0.9, 100, false, a);

   for (int i = 0; i < Slice->n_slices; i++) {
      Slice->n_macroparticles[i] = (Slice->n_slices - i);
      Slice->bin_centers[i] = 1e10 * (i + 1) / Slice->n_slices;
   }
   lhcnfb->fwhm_multi_bunch();

   auto params = std::string("../unit-tests/references/")
                 + "LHCNoiseFB/fwhm_multi_bunch/test1/";
   f_vector_t v;

   util::read_vector_from_file(v, params + "bl_meas.txt");
   //util::dump(lhcnfb->fG, "fG\n");
   auto epsilon = 1e-8;
   auto ref = v[0];
   auto real = lhcnfb->fBlMeas;
   ASSERT_NEAR(ref, real, epsilon * std::max(fabs(ref), fabs(real)));


   delete lhcnfb;
}


TEST_F(testLHCNoiseFB, track1)
{

   auto long_tracker = new RingAndRfSection();

   auto lhcnfb = new LHCNoiseFB(1.0, 0.1, 0.9, 1);

   f_vector_t res;
   for (uint i = 0; i < 100; ++i) {
      long_tracker->track();
      Slice->track();
      lhcnfb->track();
      res.push_back(lhcnfb->fX);
   }

   auto params = std::string("../unit-tests/references/")
                 + "LHCNoiseFB/track/test1/";
   f_vector_t v;

   util::read_vector_from_file(v, params + "x.txt");
   //util::dump(lhcnfb->fG, "fG\n");

   ASSERT_EQ(v.size(), res.size());

   auto epsilon = 1e-8;
   for (unsigned int i = 0; i < v.size(); ++i) {
      auto ref = v[i];
      auto real = res[i];

      ASSERT_NEAR(ref, real, epsilon * std::max(fabs(ref), fabs(real)))
            << "Testing of fX failed on i "
            << i << std::endl;
   }


   delete lhcnfb;
   delete long_tracker;
}



int main(int ac, char *av[])
{
   ::testing::InitGoogleTest(&ac, av);
   return RUN_ALL_TESTS();
}