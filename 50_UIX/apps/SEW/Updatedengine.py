#— Orchestrator
from crawler    import Crawler
from indexer    import Indexer
from ranker     import Ranker
from pagerank   import compute_pagerank
from spellcheck import SpellChecker
from database   import (init_db, save_pages, load_pages,
                        save_index, load_index,
                        save_pagerank, load_pagerank,
                        is_indexed, set_meta, get_meta)


class SearchEngine:
    def __init__(self, seed_url: str = None,
                 max_pages: int = 50,
                 force_recrawl: bool = False):

        init_db()

        self.indexer     = Indexer()
        self.spellcheck  = SpellChecker()
        self.pagerank    = {}

        already_indexed = is_indexed()

        if force_recrawl or not already_indexed:
            if seed_url:
                print(f"[Engine] Crawling from seed: {seed_url}")
                crawler = Crawler(max_pages=max_pages)
                pages   = crawler.crawl(seed_url)
                save_pages(pages)
            else:
                # Fall back to demo pages
                pages = self._demo_pages()
                save_pages(pages)

            self.indexer.index(pages)
            save_index(self.indexer.inverted_index, self.indexer.doc_freq)

            print("[Engine] Computing PageRank...")
            self.pagerank = compute_pagerank(pages)
            save_pagerank(self.pagerank)

            set_meta("seed_url", seed_url or "demo")
        else:
            print("[Engine] Loading existing index from DB...")
            pages = load_pages()
            inv_index, doc_freq = load_index()
            self.indexer.load(inv_index, doc_freq, pages)
            self.pagerank = load_pagerank()

        self.spellcheck.train(pages)
        self.ranker = Ranker(self.indexer, self.pagerank)
        print("[Engine] Ready.")

    def search(self, query: str, top_n: int = 10) -> dict:
        corrected, was_corrected = self.spellcheck.correct_query(query)

        results = self.ranker.search(corrected, top_n)
        return {
            "query"        : query,
            "corrected"    : corrected if was_corrected else None,
            "results"      : results
        }

    @staticmethod
    def _demo_pages() -> dict:
        return {
            "[example.com](https://example.com/python)": {
                "title": "Python Programming Language",
                "text" : "Python is a high-level general-purpose programming language. "
                         "It supports object-oriented, functional, and procedural paradigms. "
                         "Python is widely used in data science, machine learning, web "
                         "development, scripting, and automation tasks.",
                "links": ["[example.com](https://example.com/ml)",
                          "[example.com](https://example.com/databases)"]
            },
            "[example.com](https://example.com/linux)": {
                "title": "Linux Kernel",
                "text" : "Linux is a free open-source Unix-like operating system kernel "
                         "created by Linus Torvalds in 1991. It powers servers, embedded "
                         "systems, Android devices, and supercomputers around the world.",
                "links": ["[example.com](https://example.com/python)"]
            },
            "[example.com](https://example.com/ml)": {
                "title": "Machine Learning",
                "text" : "Machine learning is a subset of artificial intelligence enabling "
                         "systems to learn from data. Algorithms include decision trees, "
                         "neural networks, support vector machines, and k-nearest neighbors.",
                "links": ["[example.com](https://example.com/python)",
                          "[example.com](https://example.com/databases)"]
            },
            "[example.com](https://example.com/databases)": {
                "title": "Database Systems",
                "text" : "A database is an organized collection of structured data. "
                         "Relational databases use SQL. Popular systems include PostgreSQL, "
                         "MySQL, SQLite, and Oracle. NoSQL databases like MongoDB handle "
                         "unstructured data at scale.",
                "links": ["[example.com](https://example.com/python)",
                          "[example.com](https://example.com/linux)"]
            },
            "[example.com](https://example.com/networking)": {
                "title": "Computer Networking",
                "text" : "Computer networking connects devices to share resources and data. "
                         "Core protocols include TCP/IP, HTTP, DNS, and TLS. Network "
                         "topologies include star, mesh, and ring configurations.",
                "links": ["[example.com](https://example.com/linux)",
                          "[example.com](https://example.com/databases)"]
            },
        }
