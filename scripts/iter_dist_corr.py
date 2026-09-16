import re
import numpy as np
import matplotlib.pyplot as plt


INPUT_FILE = "scripts/iterations.txt"


# ------------------------------------------------------------
# Load data
# ------------------------------------------------------------

iterations = []
distances = []

with open(INPUT_FILE, "r") as file:
    for line in file:
        line = line.strip()

        if not line:
            continue

        match = re.match(r"(\d+),([\d.]+)\s*", line)

        if match:
            iterations.append(int(match.group(1)))
            distances.append(float(match.group(2)))

iterations = np.array(iterations)
distances = np.array(distances)

print(f"Loaded {len(iterations)} routes")


# ------------------------------------------------------------
# Pearson correlation
# ------------------------------------------------------------

correlation = np.corrcoef(distances, iterations)[0, 1]

print("\n--- Correlation ---")
print(f"Pearson correlation: {correlation:.4f}")


# ------------------------------------------------------------
# Linear regression
#
# iterations = slope * distance + intercept
# ------------------------------------------------------------

slope, intercept = np.polyfit(distances, iterations, 1)

predicted = slope * distances + intercept

ss_res = np.sum((iterations - predicted) ** 2)
ss_tot = np.sum((iterations - np.mean(iterations)) ** 2)

r_squared = 1 - ss_res / ss_tot

print("\n--- Linear regression ---")
print(f"Slope:     {slope:.2f} iterations / mile")
print(f"Intercept: {intercept:.2f} iterations")
print(f"R²:        {r_squared:.4f}")

print(
    f"\nRegression equation:\n"
    f"iterations = {slope:.2f} × distance + {intercept:.2f}"
)


# ------------------------------------------------------------
# Residuals
# ------------------------------------------------------------

residuals = iterations - predicted

print("\n--- Residuals ---")
print(f"Mean:   {np.mean(residuals):.2f}")
print(f"Median: {np.median(residuals):.2f}")
print(f"Min:    {np.min(residuals):.2f}")
print(f"Max:    {np.max(residuals):.2f}")


# ------------------------------------------------------------
# 99th percentile upper residual
# ------------------------------------------------------------

residual_99 = np.percentile(residuals, 99)

upper_intercept = intercept + residual_99

print("\n--- 99th percentile upper bound ---")
print(f"99th percentile residual: {residual_99:.2f}")

print(
    f"\n99% empirical upper-bound equation:\n"
    f"iterations = {slope:.2f} × distance + {upper_intercept:.2f}"
)

upper_bound = predicted + residual_99

coverage = np.mean(iterations <= upper_bound) * 100

print(f"Observed routes below bound: {coverage:.2f}%")


# ------------------------------------------------------------
# Raw iteration percentiles
# ------------------------------------------------------------

print("\n--- Iteration distribution ---")
print(f"Median:       {np.percentile(iterations, 50):.0f}")
print(f"90th:         {np.percentile(iterations, 90):.0f}")
print(f"95th:         {np.percentile(iterations, 95):.0f}")
print(f"99th:         {np.percentile(iterations, 99):.0f}")
print(f"Maximum:      {np.max(iterations):.0f}")


# ------------------------------------------------------------
# Worst routes relative to regression
# ------------------------------------------------------------

worst_indices = np.argsort(residuals)[-10:][::-1]

print("\n--- 10 routes furthest above regression ---")

for i in worst_indices:
    print(
        f"Distance: {distances[i]:8.3f} mi | "
        f"Iterations: {iterations[i]:5d} | "
        f"Predicted: {predicted[i]:7.1f} | "
        f"Residual: {residuals[i]:7.1f}"
    )


# ------------------------------------------------------------
# Plot 1: iterations vs distance
# ------------------------------------------------------------

x = np.linspace(0, distances.max(), 500)

regression_line = slope * x + intercept
upper_line = slope * x + upper_intercept

plt.figure(figsize=(10, 6))

plt.scatter(
    distances,
    iterations,
    s=25,
    alpha=0.6,
    label="Observed routes"
)

plt.plot(
    x,
    regression_line,
    linewidth=2,
    label="Linear regression"
)

plt.plot(
    x,
    upper_line,
    linestyle="--",
    linewidth=2,
    label="99th percentile upper bound"
)

plt.xlabel("Heuristic distance (miles)")
plt.ylabel("Iterations")
plt.title("GPU routing iterations vs heuristic distance")
plt.legend()
plt.grid(alpha=0.3)

plt.tight_layout()
plt.show()


# ------------------------------------------------------------
# Plot 2: residuals
# ------------------------------------------------------------

plt.figure(figsize=(10, 6))

plt.scatter(
    distances,
    residuals,
    s=25,
    alpha=0.6
)

plt.axhline(0, linewidth=1)

plt.axhline(
    residual_99,
    linestyle="--",
    linewidth=2,
    label="99th percentile residual"
)

plt.xlabel("Heuristic distance (miles)")
plt.ylabel("Residual (actual - predicted)")
plt.title("Regression residuals")
plt.legend()
plt.grid(alpha=0.3)

plt.tight_layout()
plt.show()