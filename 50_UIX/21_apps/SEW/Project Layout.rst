search_engine/
├── crawler.py
├── indexer.py
├── ranker.py
├── engine.py
├── app.py
└── templates/
    └── index.html

Updated Project Layout
search_engine/
├── Updatedcrawler.py
├── Updatedindexer.py
├── Updatedranker.py
├── pagerank.py
├── spellcheck.py
├── database.py
├── Updatedengine.py
├── Updatedapp.py
└── templates/
    └── Updatedindex.html


Here's the fully extended search engine with all four additions:

Updated Project Layout

``
searchengine/
├── crawler.py
├── indexer.py
├── ranker.py
├── pagerank.py
├── spellcheck.py
├── database.py
├── engine.py
├── app.py
└── templates/
    └── index.html
`

database.py — SQLite Persistence

`python
import sqlite3
import json
import os

DBPATH = "searchengine.db"

def getconn():
    conn = sqlite3.connect(DBPATH)
    conn.rowfactory = sqlite3.Row
    return conn

def initdb():
    with getconn() as conn:
        conn.executescript("""
            CREATE TABLE IF NOT EXISTS documents (
                url     TEXT PRIMARY KEY,
                title   TEXT,
                text    TEXT,
                links   TEXT    -- JSON list of outbound links
            );

            CREATE TABLE IF NOT EXISTS invertedindex (
                term    TEXT,
                url     TEXT,
                tf      REAL,
                PRIMARY KEY (term, url)
            );

            CREATE TABLE IF NOT EXISTS docfreq (
                term    TEXT PRIMARY KEY,
                df      INTEGER
            );

            CREATE TABLE IF NOT EXISTS pagerank (
                url     TEXT PRIMARY KEY,
                score   REAL
            );

            CREATE TABLE IF NOT EXISTS metadata (
                key     TEXT PRIMARY KEY,
                value   TEXT
            );
        """)
    print("[DB] Initialized database.")

def savepages(pages: dict):
    """Persist crawled pages."""
    with getconn() as conn:
        for url, data in pages.items():
            conn.execute("""
                INSERT OR REPLACE INTO documents (url, title, text, links)
                VALUES (?, ?, ?, ?)
            """, (
                url,
                data.get("title", ""),
                data.get("text",  ""),
                json.dumps(data.get("links", []))
            ))
    print(f"[DB] Saved {len(pages)} documents.")

def loadpages() -> dict:
    """Load all crawled pages from DB."""
    with getconn() as conn:
        rows = conn.execute(
            "SELECT url, title, text, links FROM documents"
        ).fetchall()
    pages = {}
    for row in rows:
        pages[row["url"]] = {
            "title": row["title"],
            "text" : row["text"],
            "links": json.loads(row["links"] or "[]")
        }
    return pages

def saveindex(invertedindex: dict, docfreq: dict):
    """Persist inverted index and document frequencies."""
    with getconn() as conn:
        conn.execute("DELETE FROM invertedindex")
        conn.execute("DELETE FROM docfreq")

        for term, urltf in invertedindex.items():
            for url, tf in urltf.items():
                conn.execute("""
                    INSERT OR REPLACE INTO invertedindex (term, url, tf)
                    VALUES (?, ?, ?)
                """, (term, url, tf))

        for term, df in docfreq.items():
            conn.execute("""
                INSERT OR REPLACE INTO docfreq (term, df)
                VALUES (?, ?)
            """, (term, df))

    print("[DB] Saved inverted index.")

def loadindex():
    """Load inverted index and doc freq from DB."""
    from collections import defaultdict
    invertedindex = defaultdict(dict)
    docfreq       = defaultdict(int)

    with getconn() as conn:
        for row in conn.execute("SELECT term, url, tf FROM invertedindex"):
            invertedindex[row["term"]][row["url"]] = row["tf"]
        for row in conn.execute("SELECT term, df FROM docfreq"):
            docfreq[row["term"]] = row["df"]

    return invertedindex, docfreq

def savepagerank(scores: dict):
    """Persist PageRank scores."""
    with getconn() as conn:
        conn.execute("DELETE FROM pagerank")
        for url, score in scores.items():
            conn.execute("""
                INSERT OR REPLACE INTO pagerank (url, score)
                VALUES (?, ?)
            """, (url, score))
    print("[DB] Saved PageRank scores.")

def loadpagerank() -> dict:
    """Load PageRank scores from DB."""
    with getconn() as conn:
        rows = conn.execute("SELECT url, score FROM pagerank").fetchall()
    return {row["url"]: row["score"] for row in rows}

def isindexed() -> bool:
    """Check if index already exists in DB."""
    with getconn() as conn:
        count = conn.execute(
            "SELECT COUNT() FROM invertedindex"
        ).fetchone()[0]
    return count > 0

def setmeta(key, value):
    with getconn() as conn:
        conn.execute("""
            INSERT OR REPLACE INTO metadata (key, value) VALUES (?, ?)
        """, (key, str(value)))

def getmeta(key):
    with getconn() as conn:
        row = conn.execute(
            "SELECT value FROM metadata WHERE key=?", (key,)
        ).fetchone()
    return row["value"] if row else None
`

crawler.py — Real Crawl with Link Tracking

`python
import requests
from bs4 import BeautifulSoup
from urllib.parse import urljoin, urlparse
from collections import deque

class Crawler:
    def init(self, maxpages=50):
        self.maxpages = maxpages
        self.visited   = set()
        self.pages     = {}

    def crawl(self, seedurl: str) -> dict:
        queue = deque([seedurl])
        parsedseed = urlparse(seedurl)
        base = f"{parsedseed.scheme}://{parsedseed.netloc}"

        while queue and len(self.visited) < self.maxpages:
            url = queue.popleft()
            if url in self.visited:
                continue
            try:
                resp = requests.get(url, timeout=5,
                    headers={"User-Agent": "MiniSearchBot/1.0"})
                if "text/html" not in resp.headers.get("Content-Type", ""):
                    continue

                soup  = BeautifulSoup(resp.text, "html.parser")

                for tag in soup(["script", "style", "nav", "footer", "aside"]):
                    tag.decompose()

                text  = soup.gettext(separator=" ", strip=True)
                title = soup.title.string.strip() if soup.title else url

                # Collect outbound links (same domain only)
                links = []
                for a in soup.findall("a", href=True):
                    link = urljoin(base, a["href"])
                    link = link.split("#")[0]   # strip fragments
                    if link.startswith(base) and link != url:
                        links.append(link)
                        if link not in self.visited:
                            queue.append(link)

                self.pages[url] = {
                    "title": title,
                    "text" : text,
                    "links": list(set(links))
                }
                self.visited.add(url)
                print(f"[Crawled] ({len(self.visited)}/{self.maxpages}) {url}")

            except Exception as e:
                print(f"[Skip] {url} — {e}")

        return self.pages
`

pagerank.py — PageRank Algorithm

`python
def computepagerank(pages: dict,
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

    urlset = set(urls)

    # Build adjacency: who does each page link to (filtered to known pages)
    outlinks = {}
    for url, data in pages.items():
        outlinks[url] = [l for l in data.get("links", []) if l in urlset]

    # Build reverse index: who links TO each page
    inlinks = {url: [] for url in urls}
    for url, targets in outlinks.items():
        for t in targets:
            inlinks[t].append(url)

    # Initialize ranks uniformly
    rank = {url: 1.0 / n for url in urls}

    for iteration in range(iterations):
        newrank = {}
        for url in urls:
            # Sum contributions from pages that link here
            incomingsum = 0.0
            for src in inlinks[url]:
                outcount = len(outlinks[src]) or 1
                incomingsum += rank[src] / outcount

            newrank[url] = (1 - damping) / n + damping  incomingsum

        # Check convergence
        diff = sum(abs(newrank[u] - rank[u]) for u in urls)
        rank = newrank
        if diff < tolerance:
            print(f"[PageRank] Converged at iteration {iteration + 1}")
            break

    # Normalize to [0, 1]
    maxscore = max(rank.values()) or 1
    rank = {url: score / maxscore for url, score in rank.items()}

    return rank
`

spellcheck.py — Spell Correction

`python
import re
from collections import Counter

class SpellChecker:
    """
    Peter Norvig-style spell corrector trained on the indexed corpus.
    """

    def init(self):
        self.wordcounts = Counter()

    def train(self, pages: dict):
        """Build word frequency model from crawled text."""
        for data in pages.values():
            words = re.findall(r"[a-z]+", data.get("text", "").lower())
            self.wordcounts.update(words)
        print(f"[SpellCheck] Trained on {len(self.wordcounts)} unique words.")

    def edits1(self, word: str) -> set:
        letters = "abcdefghijklmnopqrstuvwxyz"
        splits  = [(word[:i], word[i:]) for i in range(len(word) + 1)]
        deletes    = [L + R[1:]          for L, R in splits if R]
        transposes = [L + R[1] + R[0] + R[2:] for L, R in splits if len(R) > 1]
        replaces   = [L + c + R[1:]      for L, R in splits if R for c in letters]
        inserts    = [L + c + R          for L, R in splits for c in letters]
        return set(deletes + transposes + replaces + inserts)

    def edits2(self, word: str) -> set:
        return {e2 for e1 in self.edits1(word) for e2 in self.edits1(e1)}

    def known(self, words: set) -> set:
        return {w for w in words if w in self.wordcounts}

    def correct(self, word: str) -> str:
        word = word.lower()
        candidates = (
            self.known({word}) or
            self.known(self.edits1(word)) or
            self.known(self.edits2(word)) or
            {word}
        )
        return max(candidates, key=lambda w: self.wordcounts[w])

    def correctquery(self, query: str) -> tuple[str, bool]:
        """
        Returns (correctedquery, wascorrected).
        """
        words     = query.lower().split()
        corrected = [self.correct(w) for w in words]
        newquery = " ".join(corrected)
        return newquery, newquery != query.lower()
`

indexer.py — TF-IDF Indexer

`python
import re
import math
from collections import defaultdict

STOPWORDS = {
    "a","an","the","is","it","in","on","at","to","of","and","or",
    "for","with","this","that","was","are","be","has","had","have",
    "he","she","they","we","you","i","its","as","by","from","but"
}

def tokenize(text: str) -> list:
    tokens = re.findall(r"[a-z]+", text.lower())
    return [t for t in tokens if t not in STOPWORDS and len(t) > 2]

class Indexer:
    def init(self):
        self.invertedindex = defaultdict(dict)
        self.doccount      = 0
        self.docfreq       = defaultdict(int)
        self.docs           = {}

    def index(self, pages: dict):
        self.docs      = pages
        self.doccount = len(pages)

        for url, data in pages.items():
            tokens = tokenize(data["text"])
            freq   = defaultdict(int)
            for t in tokens:
                freq[t] += 1

            total = len(tokens) or 1
            for term, count in freq.items():
                self.invertedindex[term][url] = count / total

            for term in freq:
                self.docfreq[term] += 1

        print(f"[Indexed] {self.doccount} docs, "
              f"{len(self.invertedindex)} unique terms.")

    def load(self, invertedindex, docfreq, docs):
        self.invertedindex = invertedindex
        self.docfreq       = docfreq
        self.docs           = docs
        self.doccount      = len(docs)

    def idf(self, term: str) -> float:
        df = self.docfreq.get(term, 0)
        return math.log(self.doccount / df) if df else 0
`

ranker.py — TF-IDF + PageRank Combined Scorer

`python
from indexer import tokenize
from collections import defaultdict

class Ranker:
    def init(self, indexer, pagerank: dict = None, prweight: float = 0.3):
        self.indexer   = indexer
        self.pagerank  = pagerank or {}
        self.prweight = prweight   # how much PageRank influences final score

    def search(self, query: str, topn: int = 10) -> list:
        terms  = tokenize(query)
        scores = defaultdict(float)

        for term in terms:
            if term not in self.indexer.invertedindex:
                continue
            idf = self.indexer.idf(term)
            for url, tf in self.indexer.invertedindex[term].items():
                scores[url] += tf  idf

        # Blend TF-IDF with PageRank
        if self.pagerank:
            maxtfidf = max(scores.values(), default=1) or 1
            for url in scores:
                tfidfnorm = scores[url] / maxtfidf
                prscore   = self.pagerank.get(url, 0)
                scores[url] = (
                    (1 - self.prweight)  tfidfnorm +
                    self.prweight  prscore
                )

        ranked = sorted(scores.items(), key=lambda x: x[1], reverse=True)

        results = []
        for url, score in ranked[:topn]:
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
`

engine.py — Orchestrator

`python
from crawler    import Crawler
from indexer    import Indexer
from ranker     import Ranker
from pagerank   import computepagerank
from spellcheck import SpellChecker
from database   import (initdb, savepages, loadpages,
                        saveindex, loadindex,
                        savepagerank, loadpagerank,
                        isindexed, setmeta, getmeta)

class SearchEngine:
    def init(self, seedurl: str = None,
                 maxpages: int = 50,
                 forcerecrawl: bool = False):

        initdb()

        self.indexer     = Indexer()
        self.spellcheck  = SpellChecker()
        self.pagerank    = {}

        alreadyindexed = isindexed()

        if forcerecrawl or not alreadyindexed:
            if seedurl:
                print(f"[Engine] Crawling from seed: {seedurl}")
                crawler = Crawler(maxpages=maxpages)
                pages   = crawler.crawl(seedurl)
                savepages(pages)
            else:
                # Fall back to demo pages
                pages = self.demopages()
                savepages(pages)

            self.indexer.index(pages)
            saveindex(self.indexer.invertedindex, self.indexer.docfreq)

            print("[Engine] Computing PageRank...")
            self.pagerank = computepagerank(pages)
            savepagerank(self.pagerank)

            setmeta("seedurl", seedurl or "demo")
        else:
            print("[Engine] Loading existing index from DB...")
            pages = loadpages()
            invindex, docfreq = loadindex()
            self.indexer.load(invindex, docfreq, pages)
            self.pagerank = loadpagerank()

        self.spellcheck.train(pages)
        self.ranker = Ranker(self.indexer, self.pagerank)
        print("[Engine] Ready.")

    def search(self, query: str, topn: int = 10) -> dict:
        corrected, wascorrected = self.spellcheck.correctquery(query)

        results = self.ranker.search(corrected, topn)
        return {
            "query"        : query,
            "corrected"    : corrected if wascorrected else None,
            "results"      : results
        }

    @staticmethod
    def demopages() -> dict:
        return {
            "https://example.com/python": {
                "title": "Python Programming Language",
                "text" : "Python is a high-level general-purpose programming language. "
                         "It supports object-oriented, functional, and procedural paradigms. "
                         "Python is widely used in data science, machine learning, web "
                         "development, scripting, and automation tasks.",
                "links": ["https://example.com/ml",
                          "https://example.com/databases"]
            },
            "https://example.com/linux": {
                "title": "Linux Kernel",
                "text" : "Linux is a free open-source Unix-like operating system kernel "
                         "created by Linus Torvalds in 1991. It powers servers, embedded "
                         "systems, Android devices, and supercomputers around the world.",
                "links": ["https://example.com/python"]
            },
            "https://example.com/ml": {
                "title": "Machine Learning",
                "text" : "Machine learning is a subset of artificial intelligence enabling "
                         "systems to learn from data. Algorithms include decision trees, "
                         "neural networks, support vector machines, and k-nearest neighbors.",
                "links": ["https://example.com/python",
                          "https://example.com/databases"]
            },
            "https://example.com/databases": {
                "title": "Database Systems",
                "text" : "A database is an organized collection of structured data. "
                         "Relational databases use SQL. Popular systems include PostgreSQL, "
                         "MySQL, SQLite, and Oracle. NoSQL databases like MongoDB handle "
                         "unstructured data at scale.",
                "links": ["https://example.com/python",
                          "https://example.com/linux"]
            },
            "https://example.com/networking": {
                "title": "Computer Networking",
                "text" : "Computer networking connects devices to share resources and data. "
                         "Core protocols include TCP/IP, HTTP, DNS, and TLS. Network "
                         "topologies include star, mesh, and ring configurations.",
                "links": ["https://example.com/linux",
                          "https://example.com/databases"]
            },
        }
`

app.py — Flask Web UI

`python
from flask import Flask, request, rendertemplate
from engine import SearchEngine

app = Flask(name)

── Uncomment to crawl a real site ───────────────────────────────
engine = SearchEngine(seedurl="https://en.wikipedia.org/wiki/Python(programminglanguage)",
maxpages=40)
── Demo mode (no crawl needed) ──────────────────────────────────
engine = SearchEngine()

@app.route("/", methods=["GET", "POST"])
def index():
    resultdata = None
    query       = ""

    if request.method == "POST":
        query       = request.form.get("query", "").strip()
        if query:
            resultdata = engine.search(query)

    return rendertemplate("index.html",
                           data=resultdata,
                           query=query)

@app.route("/reindex", methods=["POST"])
def reindex():
    """Force a re-crawl and re-index."""
    global engine
    seed = request.form.get("seedurl", "").strip()
    engine = SearchEngine(
        seedurl=seed or None,
        maxpages=int(request.form.get("maxpages", 30)),
        forcerecrawl=True
    )
    return {"status": "ok", "message": "Re-indexed successfully."}

if name == "main":
    app.run(debug=True)
`

templates/index.html

`html
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>MiniSearch</title>
  <style>
    , ::before, ::after { box-sizing: border-box; margin: 0; padding: 0; }

    body {
      font-family: Arial, sans-serif;
      background: #f1f3f4; min-height: 100vh;
    }

    / ── Header ── /
    .header {
      background: #4285f4; padding: 28px 20px 20px;
      text-align: center; color: white;
    }
    .header h1 { font-size: 2.2rem; margin-bottom: 14px; letter-spacing: -0.5px; }

    .search-bar { display: flex; justify-content: center; gap: 8px; }
    .search-bar input {
      width: min(520px, 85vw); padding: 12px 18px;
      font-size: 1rem; border: none; border-radius: 24px; outline: none;
    }
    .search-bar button {
      padding: 12px 24px; background: white; color: #4285f4;
      border: none; border-radius: 24px; font-size: 1rem;
      cursor: pointer; font-weight: bold;
      transition: background 0.2s;
    }
    .search-bar button:hover { background: #e8eaed; }

    / ── Results container ── /
    .container { max-width: 720px; margin: 28px auto; padding: 0 16px; }

    / ── Spell correction banner ── /
    .correction {
      background: #fff3cd; border: 1px solid #ffc107;
      border-radius: 6px; padding: 10px 16px;
      margin-bottom: 16px; color: #856404; font-size: 0.95rem;
    }
    .correction strong { color: #4285f4; cursor: pointer; }

    / ── Result cards ── /
    .result-count { color: #666; font-size: 0.9rem; margin-bottom: 14px; }

    .result-card {
      background: white; border-radius: 8px;
      padding: 16px 20px; margin-bottom: 14px;
      box-shadow: 0 1px 4px rgba(0,0,0,0.08);
      transition: box-shadow 0.2s;
    }
    .result-card:hover { box-shadow: 0 3px 10px rgba(0,0,0,0.13); }

    .result-card a {
      font-size: 1.1rem; color: #1a0dab;
      text-decoration: none; font-weight: bold;
    }
    .result-card a:hover { text-decoration: underline; }

    .result-url   { color: #006621; font-size: 0.82rem; margin: 4px 0 8px; }
    .result-snippet { color: #444; font-size: 0.95rem; line-height: 1.6; }

    .result-meta {
      display: flex; gap: 16px; margin-top: 8px;
      font-size: 0.78rem; color: #999;
    }
    .badge {
      background: #e8f0fe; color: #4285f4;
      border-radius: 12px; padding: 2px 8px; font-size: 0.75rem;
    }

    / ── Empty / no results ── /
    .no-results {
      text-align: center; color: #666;
      margin-top: 60px; font-size: 1.05rem;
    }

    / ── Landing ── /
    .landing {
      text-align: center; color: #999;
      margin-top: 80px; font-size: 1rem;
    }
    .landing span { font-size: 2.5rem; display: block; margin-bottom: 12px; }
  </style>
</head>
<body>

<div class="header">
  <h1>🔍 MiniSearch</h1>
  <form method="POST">
    <div class="search-bar">
      <input type="text" name="query"
             placeholder="Search anything..."
             value="{{ query }}" autofocus>
      <button type="submit">Search</button>
    </div>
  </form>
</div>

<div class="container">

  {% if data %}

    {# ── Spell correction notice ── #}
    {% if data.corrected %}
    <div class="correction">
      Did you mean:
      <form method="POST" style="display:inline">
        <input type="hidden" name="query" value="{{ data.corrected }}">
        <button type="submit"
          style="background:none;border:none;cursor:pointer;padding:0;">
          <strong>{{ data.corrected }}</strong>
        </button>
      </form>
      ? (Showing results for <em>{{ data.corrected }}</em>)
    </div>
    {% endif %}

    {# ── Result count ── #}
    {% if data.results %}
    <div class="result-count">
      {{ data.results | length }} result(s) for
      <strong>"{{ data.corrected or data.query }}"</strong>
    </div>

    {# ── Result cards ── #}
    {% for r in data.results %}
    <div class="result-card">
      <a href="{{ r.url }}" target="blank" rel="noopener">{{ r.title }}</a>
      <div class="result-url">{{ r.url }}</div>
      <div class="result-snippet">{{ r.snippet }}</div>
      <div class="result-meta">
        <span>Relevance: <strong>{{ r.score }}</strong></span>
        <span>PageRank: <strong>{{ r.pagerank }}</strong></span>
        {% if loop.index <= 3 %}
          <span class="badge">Top result</span>
        {% endif %}
      </div>
    </div>
    {% endfor %}

    {% else %}
    <div class="no-results">
      No results found for "<strong>{{ data.corrected or data.query }}</strong>".
    </div>
    {% endif %}

  {% else %}
  <div class="landing">
    <span>🔎</span>
    Enter a query above to search the index.
  </div>
  {% endif %}

</div>
</body>
</html>
`

Install & Run

`bash
pip install flask requests beautifulsoup4
python app.py
`

Open http://127.0.0.1:5000

To crawl a real site

In app.py, replace the engine line with:

`python
engine = SearchEngine(
    seedurl="https://en.wikipedia.org/wiki/Python(programminglanguage)",
    maxpages=40
)
`

What's new vs previous version

| Feature | How it works |
|---|---|
| PageRank | Iterative link-graph algorithm, converges within ~20 iterations, blended 30% into final score |
| SQLite persistence | Full schema: documents, invertedindex, docfreq, pagerank tables — reloads on restart without re-crawling |
| Spell correction | Norvig edit-distance model trained on crawled corpus — suggests corrections and auto-searches corrected query |
| Real crawl | Uncomment seed URL in app.py; crawler tracks outbound links needed for PageRank graph |
| /reindex endpoint | POST to /reindex with seedurl + max_pages` to trigger a fresh crawl via API |