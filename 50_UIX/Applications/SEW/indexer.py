import re
import math
from collections import defaultdict


STOP_WORDS = {
    "a","an","the","is","it","in","on","at","to","of",
    "and","or","for","with","this","that","was","are","be"
}


def tokenize(text):
    tokens = re.findall(r"[a-z]+", text.lower())
    return [t for t in tokens if t not in STOP_WORDS and len(t) > 2]


class Indexer:
    def __init__(self):
        self.inverted_index = defaultdict(dict)  # term -> {url: tf}
        self.doc_count      = 0
        self.doc_freq       = defaultdict(int)   # term -> num docs containing it
        self.docs           = {}                 # url -> metadata

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
                tf = count / total
                self.inverted_index[term][url] = tf

            for term in freq:
                self.doc_freq[term] += 1

        print(f"[Indexed] {self.doc_count} documents, "
              f"{len(self.inverted_index)} unique terms")

    def idf(self, term):
        df = self.doc_freq.get(term, 0)
        if df == 0:
            return 0
        return math.log(self.doc_count / df)
