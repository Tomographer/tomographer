
// =============================================================================
// Theory for specific components -- Title Page
// =============================================================================

/** \page pageTheory Theory for specific %Tomographer components
 *
 * These pages document the theory specific to some components of the %Tomographer
 * framework.
 *
 * Specific Topics:
 *  - \subpage pageTheoryAveragedHistogram
 *
 */


// =============================================================================
// Theory for specific components -- Averaged Histogram
// =============================================================================

/** \page pageTheoryAveragedHistogram Averaged Histogram
 *
 * <h3>Averaging Raw Histograms</h3>
 *
 * Given raw histogram counts from independent experiments, we can combine them into one
 * histogram with error bars as follows. Let \f$ x_k^{(i)} \f$ be the raw histogram counts
 * in bin \f$ k\f$ of experiment \f$ i\f$ of \f$ n\f$. Then the full histogram counts
 * \f$ y_k \f$, with corresponding error bars \f$ \Delta_k \f$, is
 * \f{align*}{
 *   y_k &= \frac1n \sum_i x_k^{(i)} \ ; \\
 *   \Delta_k &= \sqrt{\frac1{n-1}\left(\langle (x_k^{(i)})^2 \rangle - \langle x_k^{(i)} \rangle^2\right)}\ ,
 * \f}
 * where \f$ \langle\cdot\rangle = \frac1n \sum_i \cdot \f$ is the average over the
 * different experiments.
 *
 *
 * <h3>Averaging Histograms Which Already Have Error Bars</h3>
 *
 * Let \f$ x_k^{(i)} \f$ raw histogram counts, and suppose that we already have error bars
 * \f$ \delta_k^{(i)} \f$ on these counts (e.g., from binning analysis).
 *
 * The combined histogram \f$ y_k \f$, with final corresponding error bars \f$ \Delta_k
 * \f$, is
 * \f{align*}{
 *   y_k = \frac1n \sum_i x_k^{(i)} \ ; \\
 *   \Delta_k = \frac1n\,\sqrt{\sum_i \left(\delta_k^{(i)}\right)^2} \ .
 * \f}
 * (propagation of errors in error analysis in physics: \f$ \Delta f = \sqrt{
 * \left(\frac{\partial f}{\partial x}\right)^2 \Delta x^2 + \cdots } \f$ ).
 *
 */
