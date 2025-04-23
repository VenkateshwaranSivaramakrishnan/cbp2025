import pandas as pd
import matplotlib.pyplot as plt
from collections import defaultdict, deque

N_HISTORY = 16
MASK_BITS = 15  # Number of lower bits to check for aliasing
MASK = (1 << MASK_BITS) - 1  # e.g., 4 → 0b1111 = 15

def analyze_branch_trace(trace_file):
    df = pd.read_csv(trace_file, header=None, names=["PC", "Target", "Predicted", "Actual"])

    summary = {
        "Always Taken": [],
        "Always Not Taken": [],
        "Loop Exit Pattern": [],
        "Alternating Pattern": [],
        "Correlated Candidates": defaultdict(list),
        "Cold Start Mispredictions": [],
        "Rarely Executed": [],
        "Conflict Miss Suspects": defaultdict(list),
        "Misprediction Rates": {},
        "Misprediction Counts": {}
    }

    history_window = deque(maxlen=N_HISTORY)
    correlation_patterns = defaultdict(lambda: defaultdict(int))
    pc_stats = defaultdict(lambda: {"T": 0, "N": 0, "Mispred": 0, "PredSeq": [], "ActualSeq": [], "IndexMap": []})

    for i, row in df.iterrows():
        pc = int(row['PC'], 16) if isinstance(row['PC'], str) else row['PC']
        predicted = int(row['Predicted'])
        actual = int(row['Actual'])

        pc_stats[pc]["T" if actual else "N"] += 1
        if predicted != actual:
            pc_stats[pc]["Mispred"] += 1

        pc_stats[pc]["PredSeq"].append(predicted)
        pc_stats[pc]["ActualSeq"].append(actual)
        pc_stats[pc]["IndexMap"].append(i)

        # Cold Start: first few mispredictions
        if len(pc_stats[pc]["ActualSeq"]) <= 3 and predicted != actual:
            summary["Cold Start Mispredictions"].append((hex(pc), i, actual))

        # Potential correlation
        #if predicted != actual:
        #    for prev_pc, prev_taken in history_window:
        #        if prev_pc != pc and prev_taken:
        #            summary["Correlated Candidates"][(hex(pc), hex(prev_pc))].append(i)
        # Track global history pattern
        history_pattern = ''.join(str(taken) for pc, taken in history_window)
        correlation_patterns[pc][(history_pattern, actual)] += 1

        history_window.append((pc, actual))

    for pc, stats in pc_stats.items():
        total = stats["T"] + stats["N"]
        mispred = stats["Mispred"]
        pred_seq = stats["PredSeq"]
        actual_seq = stats["ActualSeq"]

        hex_pc = hex(pc)

        if total > 0:
            summary["Misprediction Rates"][hex(pc)] = round(mispred / total, 3)
            summary["Misprediction Counts"][hex_pc] = mispred
        # Always Taken / Always Not Taken
        if stats["T"] == total and mispred > 0:
            summary["Always Taken"].append(hex(pc))
        elif stats["N"] == total and mispred > 0:
            summary["Always Not Taken"].append(hex(pc))

        # Alternating pattern
        if len(actual_seq) > 4:
            alternates = all(actual_seq[i] != actual_seq[i+1] for i in range(len(actual_seq)-1))
            if alternates:
                summary["Alternating Pattern"].append(hex(pc))

        # Loop Exit pattern
        if actual_seq.count(1) > 3 and actual_seq[-1] == 0 and all(actual_seq[i] for i in range(len(actual_seq)-1)):
            summary["Loop Exit Pattern"].append(hex(pc))

        # Rarely executed
        if total < 3:
            summary["Rarely Executed"].append(hex(pc))

        # Conflict Misses (same low PC bits)
        for other_pc in pc_stats:
            if pc != other_pc and (pc & MASK) == (other_pc & MASK):
                summary["Conflict Miss Suspects"][(hex(pc), hex(other_pc))].append(
                    (pc_stats[pc]["Mispred"], pc_stats[other_pc]["Mispred"])
                )
        # Correlation pattern analysis
        for pc, pattern_dict in correlation_patterns.items():
            pattern_outcomes = defaultdict(set)
            for (pattern, outcome), count in pattern_dict.items():
                pattern_outcomes[pattern].add(outcome)

            for pattern, outcomes in pattern_outcomes.items():
                if len(outcomes) == 1:
                    summary["Correlated Candidates"][hex(pc)].append(pattern)

    return summary

def save_summary(summary, filename="branch_analysis_summary.txt"):
    with open(filename, 'w') as f:
        for category, items in summary.items():
            f.write(f"=== {category} ===\n")
            if isinstance(items, dict):
                for k, v in items.items():
                    f.write(f"{k}: {v}\n")
            else:
                for item in items:
                    f.write(f"{item}\n")
            f.write("\n")

def plot_misprediction_rates(summary, output_file="misprediction_rate.png", count_output="misprediction_count.png"):
    pcs = list(summary["Misprediction Rates"].keys())
    rates = list(summary["Misprediction Rates"].values())
    counts = [summary["Misprediction Counts"].get(pc, 0) for pc in pcs]
    total_mispred = sum(counts)

    # Plot misprediction rate
    plt.figure(figsize=(14, 6))
    plt.bar(pcs, rates)
    plt.xticks(rotation=90, fontsize=6)
    plt.xlabel("PC")
    plt.ylabel("Misprediction Rate")
    plt.title("Branch Misprediction Rate per PC")
    plt.tight_layout()
    plt.savefig(output_file)
    plt.close()

    # Plot misprediction count
    plt.figure(figsize=(14, 6))
    plt.bar(pcs, counts)
    plt.xticks(rotation=90, fontsize=6)
    plt.xlabel("PC")
    plt.ylabel("Misprediction Count")
    plt.title(f"Branch Misprediction Count per PC (Total: {total_mispred})")
    plt.tight_layout()
    plt.savefig(count_output)
    plt.close()

if __name__ == "__main__":
    trace_file = "trace_debug.txt"
    print(f"Analyzing {trace_file}...")

    summary = analyze_branch_trace(trace_file)
    save_summary(summary)
    plot_misprediction_rates(summary)

    print("✅ Analysis complete.")
    print("➡️ Summary saved to 'branch_analysis_summary.txt'")
    print("➡️ Plot saved to 'misprediction_rate.png'")
