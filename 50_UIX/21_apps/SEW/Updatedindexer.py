#— TF-IDF Indexer
import re
import math
from collections import defaultdict


STOP_WORDS = {
    "a","an","the","is","it","in","on","at","to","of","and","or",
    "for","with","this","that","was","are","be","has","had","have",
    "he","she","they","we","you","i","its","as","by","from","but"
}


def tokenize(text: str) -> list:
    tokens = re.findall(r"[a-z]+", text.lower())
    return [t for t in tokens if t not in STOP_WORDS and len(t) > 2]


class Indexer:
    def __init__(self):
        self.inverted_index = defaultdict(dict)
        self.doc_count      = 0
        self.doc_freq       = defaultdict(int)
        self.docs           = {}

    def index(self, pages: dict):
        self.docs      = pages
        self.doc_count = len(pages)

        for url, data in pages.items():
            tokens = tokenize(data["text"])
            freq   = defaultdict(int)
            for t in tokens:
                freq[t] += 1

            total = len(tokens) or 1
            for term, count in freq.items():
                self.inverted_index[term][url] = count / total

            for term in freq:
                self.doc_freq[term] += 1

        print(f"[Indexed] {self.doc_count} docs, "
              f"{len(self.inverted_index)} unique terms.")

    def load(self, inverted_index, doc_freq, docs):
        self.inverted_index = inverted_index
        self.doc_freq       = doc_freq
        self.docs           = docs
        self.doc_count      = len(docs)

    def idf(self, term: str) -> float:
        df = self.doc_freq.get(term, 0)
        return math.log(self.doc_count / df) if df else 0

