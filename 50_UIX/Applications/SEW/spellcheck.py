#— Spell Correction
import re
from collections import Counter


class SpellChecker:
    """
    Peter Norvig-style spell corrector trained on the indexed corpus.
    """

    def __init__(self):
        self.word_counts = Counter()

    def train(self, pages: dict):
        """Build word frequency model from crawled text."""
        for data in pages.values():
            words = re.findall(r"[a-z]+", data.get("text", "").lower())
            self.word_counts.update(words)
        print(f"[SpellCheck] Trained on {len(self.word_counts)} unique words.")

    def _edits1(self, word: str) -> set:
        letters = "abcdefghijklmnopqrstuvwxyz"
        splits  = [(word[:i], word[i:]) for i in range(len(word) + 1)]
        deletes    = [L + R[1:]          for L, R in splits if R]
        transposes = [L + R[1] + R[0] + R[2:] for L, R in splits if len(R) > 1]
        replaces   = [L + c + R[1:]      for L, R in splits if R for c in letters]
        inserts    = [L + c + R          for L, R in splits for c in letters]
        return set(deletes + transposes + replaces + inserts)

    def _edits2(self, word: str) -> set:
        return {e2 for e1 in self._edits1(word) for e2 in self._edits1(e1)}

    def _known(self, words: set) -> set:
        return {w for w in words if w in self.word_counts}

    def correct(self, word: str) -> str:
        word = word.lower()
        candidates = (
            self._known({word}) or
            self._known(self._edits1(word)) or
            self._known(self._edits2(word)) or
            {word}
        )
        return max(candidates, key=lambda w: self.word_counts[w])

    def correct_query(self, query: str) -> tuple[str, bool]:
        """
        Returns (corrected_query, was_corrected).
        """
        words     = query.lower().split()
        corrected = [self.correct(w) for w in words]
        new_query = " ".join(corrected)
        return new_query, new_query != query.lower()
