require_relative 'pra32-u2-generate-common'

# Osc 1 Tri: the weights of the corrections at a peak of the Triangle Wave, where the folded
# wave turns back, for the 4 samples around it (the one before, this one, the next, and the one after),
# by the time t from this sample to the peak (0 to 1, 64 steps, and a guard): the jump of the slope
# (the 4-point PolyBLAMP) and that of the 3rd derivative, both by the cubic B-spline (4 points)
#
# The B-spline integrated m times gives the band-limited x^(m - 1) / (m - 1)!, but for m above 2, it
# also adds the lower powers by the moments of the B-spline (2nd: 1 / 3), which are taken out by the
# smoothed ones (so that the correction ends within the 4 samples)

def b_spline_integrated(x, m)  # the cubic B-spline integrated m times
  (0..4).sum { |k| (-1) ** k * [1, 4, 6, 4, 1][k] * ([x + 2.0 - k, 0.0].max ** (3 + m)) } / (1..(3 + m)).inject(:*)
end

def residual(x, m)
  r = b_spline_integrated(x, m) - ([x, 0.0].max ** (m - 1)) / (1..(m - 1)).inject(1, :*)
  r -= (1.0 / 3.0) / 2.0 * b_spline_integrated(x, 2) if m == 4
  r
end

TRI_FOLD_WEIGHT_BITS = 16

write_header("pra32-u2-osc-tri-fold-table.h") do |file|
  weights = (0..65).flat_map do |i|
    t = [i, 64].min / 64.0
    [2, 4].flat_map { |m| [-1, 0, 1, 2].map { |o| (residual(o - t, m) * (1 << TRI_FOLD_WEIGHT_BITS)).round } }
  end
  write_array(file, "const int16_t g_osc_tri_fold_weight_table[(64 + 2) * 8]", weights, per_line: 8, format: "%+6d")
end
