#crawler.py — Real Crawl with Link Tracking
import requests
from bs4 import BeautifulSoup
from urllib.parse import urljoin, urlparse
from collections import deque


class Crawler:
    def __init__(self, max_pages=50):
        self.max_pages = max_pages
        self.visited   = set()
        self.pages     = {}

    def crawl(self, seed_url: str) -> dict:
        queue = deque([seed_url])
        parsed_seed = urlparse(seed_url)
        base = f"{parsed_seed.scheme}://{parsed_seed.netloc}"

        while queue and len(self.visited) < self.max_pages:
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

                text  = soup.get_text(separator=" ", strip=True)
                title = soup.title.string.strip() if soup.title else url

                # Collect outbound links (same domain only)
                links = []
                for a in soup.find_all("a", href=True):
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
                print(f"[Crawled] ({len(self.visited)}/{self.max_pages}) {url}")

            except Exception as e:
                print(f"[Skip] {url} — {e}")

        return self.pages
