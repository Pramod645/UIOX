import requests
from bs4 import BeautifulSoup
from urllib.parse import urljoin, urlparse
from collections import deque


class Crawler:
    def __init__(self, max_pages=50):
        self.max_pages   = max_pages
        self.visited     = set()
        self.pages       = {}   # url -> text content

    def crawl(self, seed_url):
        queue = deque([seed_url])
        base  = "{0.scheme}://{0.netloc}".format(urlparse(seed_url))

        while queue and len(self.visited) < self.max_pages:
            url = queue.popleft()
            if url in self.visited:
                continue
            try:
                resp = requests.get(url, timeout=5)
                if "text/html" not in resp.headers.get("Content-Type", ""):
                    continue
                soup = BeautifulSoup(resp.text, "html.parser")

                # Extract visible text
                for tag in soup(["script", "style", "nav", "footer"]):
                    tag.decompose()
                text  = soup.get_text(separator=" ", strip=True)
                title = soup.title.string if soup.title else url

                self.pages[url] = {"title": title, "text": text}
                self.visited.add(url)
                print(f"[Crawled] {url}")

                # Collect same-domain links
                for a in soup.find_all("a", href=True):
                    link = urljoin(base, a["href"])
                    if link.startswith(base) and link not in self.visited:
                        queue.append(link)

            except Exception as e:
                print(f"[Skip] {url} — {e}")

        return self.pages
