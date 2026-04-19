#— PageRank Algorithm
def compute_pagerank(pages: dict,
                     damping: float = 0.85,
                     iterations: int = 50,
                     tolerance: float = 1e-6) -> dict:
    """
    Compute PageRank for all crawled pages.

    pages: { url: { 'links': [url, ...], ... } }
    """
    urls    = list(pages.keys())
    n       = len(urls)
    if n == 0:
        return {}

    url_set = set(urls)

    # Build adjacency: who does each page link to (filtered to known pages)
    out_links = {}
    for url, data in pages.items():
        out_links[url] = [l for l in data.get("links", []) if l in url_set]

    # Build reverse index: who links TO each page
    in_links = {url: [] for url in urls}
    for url, targets in out_links.items():
        for t in targets:
            in_links[t].append(url)

    # Initialize ranks uniformly
    rank = {url: 1.0 / n for url in urls}

    for iteration in range(iterations):
        new_rank = {}
        for url in urls:
            # Sum contributions from pages that link here
            incoming_sum = 0.0
            for src in in_links[url]:
                out_count = len(out_links[src]) or 1
                incoming_sum += rank[src] / out_count

            new_rank[url] = (1 - damping) / n + damping * incoming_sum

        # Check convergence
        diff = sum(abs(new_rank[u] - rank[u]) for u in urls)
        rank = new_rank
        if diff < tolerance:
            print(f"[PageRank] Converged at iteration {iteration + 1}")
            break

    # Normalize to [0, 1]
    max_score = max(rank.values()) or 1
    rank = {url: score / max_score for url, score in rank.items()}

    return rank
