// Cache name with version
const CACHE_NAME = 'vite-app-cache-v1';

// Install event handler
self.addEventListener('install', (event) => {
  console.log('Service Worker installing...');
  
  // Skip waiting to activate immediately
  self.skipWaiting();
  
  // We'll dynamically cache files as they're requested rather than 
  // specifying a predefined list, since Vite uses hashed filenames in production
});

// Activate event handler
self.addEventListener('activate', (event) => {
  console.log('Service Worker activating...');
  
  // Clean up old caches
  event.waitUntil(
    caches.keys().then((cacheNames) => {
      return Promise.all(
        cacheNames.filter((name) => name !== CACHE_NAME)
          .map((name) => caches.delete(name))
      );
    }).then(() => {
      // Take control of all clients immediately
      return self.clients.claim();
    })
  );
});

// Fetch event handler
self.addEventListener('fetch', (event) => {
  // Only handle GET requests
  if (event.request.method !== 'GET') return;
  
  // Handle fetch event
  event.respondWith(
    // Try to get the response from the cache first
    caches.match(event.request).then((cachedResponse) => {
      // Return cached response if found
      if (cachedResponse) {
        return cachedResponse;
      }
      
      // Otherwise fetch from network
      return fetch(event.request).then((networkResponse) => {
        // Don't cache if not a valid response
        if (!networkResponse || networkResponse.status !== 200 || networkResponse.type !== 'basic') {
          return networkResponse;
        }
        
        // Clone the response
        const responseToCache = networkResponse.clone();
        
        // Open cache and store the response
        caches.open(CACHE_NAME).then((cache) => {
          console.log('Caching new resource:', event.request.url);
          cache.put(event.request, responseToCache);
        });
        
        return networkResponse;
      });
    }).catch(() => {
      // For navigation requests, serve the index page as fallback
      if (event.request.mode === 'navigate') {
        return caches.match('/index.html');
      }
      // Otherwise fail
      return new Response('Network error happened', {
        status: 408,
        headers: { 'Content-Type': 'text/plain' }
      });
    })
  );
});