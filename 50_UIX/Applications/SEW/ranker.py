from indexer import tokenize
from collections import defaultdict


class Ranker:
    def __init__(self, indexer):
        self.indexer = indexer

    def search(self, query: str, top_n=10):
        terms   = tokenize(query)
        scores  = defaultdict(float)

        for term in terms:
            if term not in self.indexer.inverted_index:
                continue
            idf = self.indexer.idf(term)
            for url, tf in self.indexer.inverted_index[term].items():
                scores[url] += tf * idf   # TF-IDF score

        # Sort by score descending
        ranked = sorted(scores.items(), key=lambda x: x[1], reverse=True)

        results = []
        for url, score in ranked[:top_n]:
            meta = self.indexer.docs.get(url, {})
            # Snippet: first 200 chars of text
            snippet = meta.get("text", "")[:200].strip() + "..."
            results.append({
                "url"    : url,
                "title"  : meta.get("title", url),
                "snippet": snippet,
                "score"  : round(score, 4)
            })
        return results
