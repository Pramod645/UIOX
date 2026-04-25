from crawler import Crawler
from indexer import Indexer
from ranker  import Ranker


class SearchEngine:
    def __init__(self, seed_url=None, max_pages=50, pages=None):
        self.indexer = Indexer()
        self.ranker  = None

        if pages:
            # Use pre-supplied pages dict directly
            self.indexer.index(pages)
        elif seed_url:
            crawler = Crawler(max_pages=max_pages)
            pages   = crawler.crawl(seed_url)
            self.indexer.index(pages)

        self.ranker = Ranker(self.indexer)

    def search(self, query, top_n=10):
        return self.ranker.search(query, top_n)
