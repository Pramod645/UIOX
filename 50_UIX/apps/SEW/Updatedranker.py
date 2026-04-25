#— TF-IDF + PageRank Combined Scorer
from indexer import tokenize
from collections import defaultdict


class Ranker:
    def __init__(self, indexer, pagerank: dict = None, pr_weight: float = 0.3):
        self.indexer   = indexer
        self.pagerank  = pagerank or {}
        self.pr_weight = pr_weight   # how much PageRank influences final score

    def search(self, query: str, top_n: int = 10) -> list:
        terms  = tokenize(query)
        scores = defaultdict(float)

        for term in terms:
            if term not in self.indexer.inverted_index:
                continue
            idf = self.indexer.idf(term)
            for url, tf in self.indexer.inverted_index[term].items():
                scores[url] += tf * idf

        # Blend TF-IDF with PageRank
        if self.pagerank:
            max_tfidf = max(scores.values(), default=1) or 1
            for url in scores:
                tfidf_norm = scores[url] / max_tfidf
                pr_score   = self.pagerank.get(url, 0)
                scores[url] = (
                    (1 - self.pr_weight) * tfidf_norm +
                    self.pr_weight * pr_score
                )

        ranked = sorted(scores.items(), key=lambda x: x[1], reverse=True)

        results = []
        for url, score in ranked[:top_n]:
            meta    = self.indexer.docs.get(url, {})
            snippet = meta.get("text", "")[:220].strip() + "..."
            results.append({
                "url"     : url,
                "title"   : meta.get("title", url),
                "snippet" : snippet,
                "score"   : round(score, 4),
                "pagerank": round(self.pagerank.get(url, 0), 4)
            })
        return results
