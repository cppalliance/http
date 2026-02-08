/* GCOVR Custom JavaScript - Tree View & Interactivity */

(function() {
  'use strict';

  // Wait for DOM ready
  document.addEventListener('DOMContentLoaded', function() {
    initTheme();
    initSidebar();
    initSidebarResize();
    initMobileMenu();
    initFileTree();
    initNavOverride();
    initBreadcrumbs();
    initSearch();
    initFunctionRows();
    initSorting();
    initToggleButtons();
    initCoverageNav();
    initTreeControls();
    initViewToggle();
    initSettingsDropdown();
    initTlaNavigation();
    initLineHighlight();
    initColumnToggles();
    initPopupResize();
    initFileNavTooltips();
    initFileNavKeys();
    initFunctionListPersistence();

    // Reveal page now that all init is done
    document.documentElement.classList.remove('no-transitions');

    // Prefetch linked pages on hover for instant navigation
    initPrefetch();
  });

  // ===========================================
  // Breadcrumb Links
  // ===========================================

  // Find a node in the tree by its link (HTML filename) and return
  // the full ancestor path as an array of nodes from root to target.
  function findPathInTree(nodes, targetLink) {
    for (var i = 0; i < nodes.length; i++) {
      var node = nodes[i];
      if (node.link === targetLink) {
        return [node];
      }
      if (node.children) {
        var childPath = findPathInTree(node.children, targetLink);
        if (childPath) {
          return [node].concat(childPath);
        }
      }
    }
    return null;
  }

  function initBreadcrumbs() {
    var currentSpan = document.querySelector('.breadcrumb .current');
    if (!currentSpan || !window.GCOVR_TREE_DATA) {
      if (currentSpan) currentSpan.classList.add('ready');
      return;
    }

    // Find current page in tree by its HTML filename — this is unambiguous
    // since each page only appears once in the tree.
    var currentPage = window.location.pathname.split('/').pop() || 'index.html';
    var treePath = findPathInTree(window.GCOVR_TREE_DATA, currentPage);

    if (!treePath || treePath.length === 0) {
      currentSpan.classList.add('ready');
      return;
    }

    // Build breadcrumb from the tree path (ancestor nodes → current node)
    var fragment = document.createDocumentFragment();
    var matchedSegments = [];

    // Fill an element with the segments of a (possibly joined) name like
    // "boost/url", rendering "boost", a separator, "url". Used so a joined
    // directory shows its segments inline yet remains one hyperlink target.
    function appendSegments(parentEl, name) {
      var segments = name.split('/');
      for (var k = 0; k < segments.length; k++) {
        if (k > 0) {
          var inner = document.createElement('span');
          inner.className = 'separator';
          inner.textContent = '/';
          parentEl.appendChild(inner);
        }
        parentEl.appendChild(document.createTextNode(segments[k]));
      }
    }

    for (var i = 0; i < treePath.length; i++) {
      var node = treePath[i];
      var isLast = (i === treePath.length - 1);

      if (i > 0) {
        var sep = document.createElement('span');
        sep.className = 'separator';
        sep.textContent = '/';
        fragment.appendChild(sep);
      }

      matchedSegments.push(node.name);

      if (node.link && !isLast) {
        var a = document.createElement('a');
        a.href = node.link;
        appendSegments(a, node.name);
        fragment.appendChild(a);
      } else {
        var span = document.createElement('span');
        span.className = 'current-file';
        appendSegments(span, node.name);
        fragment.appendChild(span);
      }
    }

    currentSpan.innerHTML = '';
    currentSpan.appendChild(fragment);
    currentSpan.classList.add('ready');

    // Update source-filename to match breadcrumb path
    var sourceFilename = document.querySelector('.source-filename');
    if (sourceFilename) {
      sourceFilename.textContent = matchedSegments.join('/');
    }
  }

  // ===========================================
  // Theme Toggle
  // ===========================================

  function initTheme() {
    const toggle = document.getElementById('theme-toggle');
    const iconSun = toggle ? toggle.querySelector('.icon-sun') : null;
    const iconMoon = toggle ? toggle.querySelector('.icon-moon') : null;

    // Get system preference
    function getSystemTheme() {
      return window.matchMedia('(prefers-color-scheme: light)').matches ? 'light' : 'dark';
    }

    // Get effective theme: saved preference or OS default
    function getEffectiveTheme() {
      var saved = localStorage.getItem('gcovr-theme');
      return (saved === 'light' || saved === 'dark') ? saved : getSystemTheme();
    }

    // Apply theme to document
    function applyTheme(theme) {
      document.documentElement.setAttribute('data-theme', theme);
      if (iconSun) iconSun.style.display = (theme === 'dark') ? 'block' : 'none';
      if (iconMoon) iconMoon.style.display = (theme === 'light') ? 'block' : 'none';
    }

    // Apply current theme
    applyTheme(getEffectiveTheme());

    // Listen for system theme changes — only apply if no stored preference
    window.matchMedia('(prefers-color-scheme: light)').addEventListener('change', function() {
      var saved = localStorage.getItem('gcovr-theme');
      if (saved !== 'light' && saved !== 'dark') {
        applyTheme(getSystemTheme());
      }
    });

    // Toggle between light and dark on click
    if (toggle) {
      toggle.addEventListener('click', function() {
        var current = getEffectiveTheme();
        var next = (current === 'dark') ? 'light' : 'dark';
        localStorage.setItem('gcovr-theme', next);
        applyTheme(next);
      });
    }
  }

  // ===========================================
  // Tree Controls (Expand/Collapse All)
  // ===========================================

  function initTreeControls() {
    var expandBtn = document.getElementById('expand-all');
    var collapseBtn = document.getElementById('collapse-all');

    if (expandBtn) {
      expandBtn.addEventListener('click', function() {
        document.querySelectorAll('.tree-item').forEach(function(item) {
          if (!item.classList.contains('no-children')) {
            item.classList.add('expanded');
            var toggle = item.querySelector(':scope > .tree-item-header > .tree-folder-toggle');
            if (toggle) toggle.textContent = '−';
          }
        });
        saveExpandedFolders();
      });
    }

    if (collapseBtn) {
      collapseBtn.addEventListener('click', function() {
        document.querySelectorAll('.tree-item').forEach(function(item) {
          item.classList.remove('expanded');
          var toggle = item.querySelector(':scope > .tree-item-header > .tree-folder-toggle');
          if (toggle) toggle.textContent = '+';
        });
        saveExpandedFolders();
      });
    }
  }

  // ===========================================
  // Sidebar Toggle
  // ===========================================

  function initSidebar() {
    const sidebar = document.getElementById('sidebar');
    const toggle = document.getElementById('sidebar-toggle');
    const header = sidebar ? sidebar.querySelector('.sidebar-header') : null;

    if (!sidebar) return;

    // Load saved state
    const isCollapsed = localStorage.getItem('sidebar-collapsed') === 'true';
    if (isCollapsed) {
      sidebar.classList.add('collapsed');
    }

    // Toggle button
    if (toggle) {
      toggle.addEventListener('click', function() {
        sidebar.classList.toggle('collapsed');
        sidebar.classList.remove('hover-expand');
        var isNowCollapsed = sidebar.classList.contains('collapsed');
        localStorage.setItem('sidebar-collapsed', isNowCollapsed);
        // Restore custom width when un-collapsing
        if (!isNowCollapsed) {
          var savedWidth = localStorage.getItem('gcovr-sidebar-width');
          if (savedWidth) {
            document.documentElement.style.setProperty('--sidebar-width', savedWidth + 'px');
          }
        }
      });
    }

    // Hover expand - expands when hovering sidebar content (not header or no-expand zones)
    var hoverTimeout = null;
    var HOVER_DELAY = 150; // ms delay before expanding
    var isOverContent = false;

    // Check if element is within a no-expand zone
    function isInNoExpandZone(el) {
      while (el && el !== sidebar) {
        if (el.classList && el.classList.contains('no-expand')) {
          return true;
        }
        el = el.parentElement;
      }
      return false;
    }

    function scheduleExpand() {
      if (hoverTimeout) return; // already scheduled
      if (sidebar.classList.contains('hover-expand')) return; // already expanded
      hoverTimeout = setTimeout(function() {
        if (isOverContent) {
          sidebar.classList.add('hover-expand');
        }
        hoverTimeout = null;
      }, HOVER_DELAY);
    }

    function cancelExpand() {
      if (hoverTimeout) {
        clearTimeout(hoverTimeout);
        hoverTimeout = null;
      }
      sidebar.classList.remove('hover-expand');
    }

    sidebar.addEventListener('mouseenter', function(e) {
      if (!sidebar.classList.contains('collapsed')) return;
      // Check if entering over content area (not header or no-expand zones)
      if (!header.contains(e.target) && !isInNoExpandZone(e.target)) {
        isOverContent = true;
        scheduleExpand();
      }
    });

    sidebar.addEventListener('mousemove', function(e) {
      if (!sidebar.classList.contains('collapsed')) return;
      var wasOverContent = isOverContent;
      isOverContent = !header.contains(e.target) && !isInNoExpandZone(e.target);

      if (isOverContent && !wasOverContent && !sidebar.classList.contains('hover-expand')) {
        scheduleExpand();
      }
    });

    sidebar.addEventListener('mouseleave', function() {
      isOverContent = false;
      cancelExpand();
    });
  }

  // ===========================================
  // Sidebar Resize
  // ===========================================

  function initSidebarResize() {
    var sidebar = document.getElementById('sidebar');
    var handle = document.getElementById('sidebar-resize-handle');
    if (!sidebar || !handle) return;

    var MIN_WIDTH = 200;
    var startX, startWidth;

    // Restore saved width
    var savedWidth = localStorage.getItem('gcovr-sidebar-width');
    if (savedWidth && !sidebar.classList.contains('collapsed')) {
      var w = parseInt(savedWidth, 10);
      if (w >= MIN_WIDTH) {
        document.documentElement.style.setProperty('--sidebar-width', w + 'px');
      }
    }

    function getMaxWidth() {
      return Math.floor(window.innerWidth * 0.5);
    }

    function onMouseMove(e) {
      var newWidth = startWidth + (e.clientX - startX);
      var maxW = getMaxWidth();
      if (newWidth < MIN_WIDTH) newWidth = MIN_WIDTH;
      if (newWidth > maxW) newWidth = maxW;
      document.documentElement.style.setProperty('--sidebar-width', newWidth + 'px');
    }

    function onMouseUp() {
      document.body.classList.remove('sidebar-resizing');
      document.removeEventListener('mousemove', onMouseMove);
      document.removeEventListener('mouseup', onMouseUp);
      // Save the current width
      var computed = parseInt(getComputedStyle(sidebar).width, 10);
      localStorage.setItem('gcovr-sidebar-width', computed);
    }

    handle.addEventListener('mousedown', function(e) {
      if (sidebar.classList.contains('collapsed')) return;
      e.preventDefault();
      startX = e.clientX;
      startWidth = parseInt(getComputedStyle(sidebar).width, 10);
      document.body.classList.add('sidebar-resizing');
      document.addEventListener('mousemove', onMouseMove);
      document.addEventListener('mouseup', onMouseUp);
    });

    // Double-click to reset to default width
    var DEFAULT_WIDTH = 320;
    handle.addEventListener('dblclick', function() {
      if (sidebar.classList.contains('collapsed')) return;
      document.documentElement.style.setProperty('--sidebar-width', DEFAULT_WIDTH + 'px');
      localStorage.setItem('gcovr-sidebar-width', DEFAULT_WIDTH);
    });
  }

  // ===========================================
  // Mobile Menu
  // ===========================================

  function initMobileMenu() {
    var sidebar = document.getElementById('sidebar');
    var menuBtn = document.getElementById('mobile-menu-btn');
    var backdrop = document.getElementById('sidebar-backdrop');

    if (!menuBtn || !sidebar) return;

    // Open sidebar on hamburger click
    menuBtn.addEventListener('click', function() {
      sidebar.classList.add('mobile-open');
    });

    // Close on backdrop click
    if (backdrop) {
      backdrop.addEventListener('click', function() {
        sidebar.classList.remove('mobile-open');
      });
    }

    // Close when clicking a navigation link
    sidebar.addEventListener('click', function(e) {
      if (e.target.closest('a[href]')) {
        sidebar.classList.remove('mobile-open');
      }
    });

    // Close on escape key
    document.addEventListener('keydown', function(e) {
      if (e.key === 'Escape' && sidebar.classList.contains('mobile-open')) {
        sidebar.classList.remove('mobile-open');
      }
    });
  }

  // ===========================================
  // File Tree - Load from tree.json
  // ===========================================

  function initFileTree() {
    var treeContainer = document.getElementById('file-tree');
    if (!treeContainer) return;

    renderTree(treeContainer, window.GCOVR_TREE_DATA);
  }

  // Save expanded folder paths to localStorage
  function saveExpandedFolders() {
    var paths = [];
    document.querySelectorAll('.tree-item.expanded[data-tree-path]').forEach(function(el) {
      paths.push(el.getAttribute('data-tree-path'));
    });
    localStorage.setItem('gcovr-expanded-folders', JSON.stringify(paths));
  }

  function renderTree(container, tree) {
    container.innerHTML = '';

    if (!tree || tree.length === 0) {
      container.innerHTML = '<div class="tree-loading">No files found</div>';
      return;
    }

    tree.forEach(function(item) {
      container.appendChild(createTreeItem(item, ''));
    });

    // Auto-expand to current file and highlight it
    expandToCurrentFile(container);
  }

  function expandToCurrentFile(container) {
    // Get current page filename
    var currentPage = window.location.pathname.split('/').pop() || 'index.html';

    // Find the link matching current page
    var currentLink = container.querySelector('a[href="' + currentPage + '"]');

    if (currentLink) {
      // Mark as active
      var treeItem = currentLink.closest('.tree-item');
      if (treeItem) {
        treeItem.classList.add('active');
      }

      // Expand all parent folders
      var parent = currentLink.closest('.tree-children');
      while (parent) {
        var parentItem = parent.closest('.tree-item');
        if (parentItem) {
          parentItem.classList.add('expanded');
          var toggle = parentItem.querySelector(':scope > .tree-item-header > .tree-folder-toggle');
          if (toggle) toggle.textContent = '−';
        }
        parent = parentItem ? parentItem.parentElement.closest('.tree-children') : null;
      }
    }

    // Restore previously expanded folders from localStorage
    try {
      var saved = localStorage.getItem('gcovr-expanded-folders');
      if (saved) {
        var paths = JSON.parse(saved);
        paths.forEach(function(path) {
          var el = container.querySelector('.tree-item[data-tree-path="' + CSS.escape(path) + '"]');
          if (el && !el.classList.contains('no-children')) {
            el.classList.add('expanded');
            var toggle = el.querySelector(':scope > .tree-item-header > .tree-folder-toggle');
            if (toggle) toggle.textContent = '−';
          }
        });
      }
    } catch (e) {
      // Ignore localStorage errors
    }

    // Scroll active item into view instantly
    if (currentLink) {
      currentLink.scrollIntoView({ block: 'center', behavior: 'instant' });
    }
  }

  // Clean relative path prefixes like '../../../' from names
  function cleanPathName(name) {
    if (!name) return 'unknown';
    // Remove leading ./ or ../
    while (name.indexOf('./') === 0 || name.indexOf('../') === 0) {
      if (name.indexOf('./') === 0) {
        name = name.substring(2);
      } else if (name.indexOf('../') === 0) {
        name = name.substring(3);
      }
    }
    return name || 'unknown';
  }

  // Get just the filename from a path
  function getDisplayName(name) {
    var cleaned = cleanPathName(name);
    var lastSlash = cleaned.lastIndexOf('/');
    return lastSlash >= 0 ? cleaned.substring(lastSlash + 1) : cleaned;
  }

  function createTreeItem(item, parentPath) {
    var hasChildren = item.children && item.children.length > 0;
    var isDirectory = item.isDirectory || hasChildren;
    var cleanedName = cleanPathName(item.name);
    var treePath = parentPath ? (parentPath + '/' + cleanedName) : cleanedName;

    var div = document.createElement('div');
    div.className = 'tree-item' + (isDirectory ? ' is-folder' : '') + (hasChildren ? '' : ' no-children');
    div.setAttribute('data-tree-path', treePath);

    var header = document.createElement('div');
    header.className = 'tree-item-header';
    var toggle = null;

    // Toggle button (+/-) for folders with children
    if (hasChildren) {
      toggle = document.createElement('button');
      toggle.className = 'tree-folder-toggle';
      toggle.textContent = '+';
      toggle.setAttribute('aria-label', 'Toggle folder');
      toggle.addEventListener('click', function(e) {
        e.stopPropagation();
        e.preventDefault();
        var isExpanded = div.classList.toggle('expanded');
        toggle.textContent = isExpanded ? '−' : '+';
        saveExpandedFolders();
      });
      header.appendChild(toggle);

      // Make entire header clickable to expand/collapse
      header.style.cursor = 'pointer';
      header.addEventListener('click', function(e) {
        // If clicking directly on a link, let it navigate
        if (e.target.closest('a')) return;
        e.preventDefault();
        var isExpanded = div.classList.toggle('expanded');
        toggle.textContent = isExpanded ? '−' : '+';
        saveExpandedFolders();
      });
    } else {
      var spacer = document.createElement('span');
      spacer.className = 'tree-spacer';
      header.appendChild(spacer);
    }

    // Icon - different for folders vs files
    var icon = document.createElement('span');
    if (isDirectory) {
      icon.className = 'tree-icon tree-icon-folder';
      icon.innerHTML = '<svg viewBox="0 0 16 16" width="16" height="16"><path fill="currentColor" d="M1.75 1A1.75 1.75 0 000 2.75v10.5C0 14.216.784 15 1.75 15h12.5A1.75 1.75 0 0016 13.25v-8.5A1.75 1.75 0 0014.25 3H7.5a.25.25 0 01-.2-.1l-.9-1.2C6.07 1.26 5.55 1 5 1H1.75z"/></svg>';
    } else {
      icon.className = 'tree-icon tree-icon-file';
      icon.innerHTML = '<svg viewBox="0 0 16 16" width="16" height="16"><path fill="currentColor" d="M3.75 1.5a.25.25 0 00-.25.25v12.5c0 .138.112.25.25.25h9.5a.25.25 0 00.25-.25V6h-2.75A1.75 1.75 0 019 4.25V1.5H3.75zm6.75.062V4.25c0 .138.112.25.25.25h2.688l-2.938-2.938zM2 1.75C2 .784 2.784 0 3.75 0h6.586c.464 0 .909.184 1.237.513l2.914 2.914c.329.328.513.773.513 1.237v9.586A1.75 1.75 0 0113.25 16h-9.5A1.75 1.75 0 012 14.25V1.75z"/></svg>';
    }
    header.appendChild(icon);

    // Label (with link if available)
    // Use the full cleaned name so joined-dir entries like "boost/url"
    // display as a single multi-segment label in the sidebar.
    var displayName = cleanPathName(item.name);
    var tooltipText = cleanPathName(item.fullPath || item.name);
    var label = document.createElement('span');
    label.className = 'tree-label';
    label.title = tooltipText;
    if (item.link) {
      var link = document.createElement('a');
      link.href = item.link;
      link.textContent = displayName;
      link.title = tooltipText;
      label.appendChild(link);
    } else {
      label.textContent = displayName;
    }
    header.appendChild(label);

    div.appendChild(header);

    // Children container (for expand/collapse)
    if (hasChildren) {
      var childrenWrapper = document.createElement('div');
      childrenWrapper.className = 'tree-children';

      var childrenInner = document.createElement('div');
      childrenInner.className = 'tree-children-inner';
      item.children.forEach(function(child) {
        childrenInner.appendChild(createTreeItem(child, treePath));
      });

      childrenWrapper.appendChild(childrenInner);
      div.appendChild(childrenWrapper);
    }

    return div;
  }

  // ===========================================
  // Search
  // ===========================================

  function initSearch() {
    const searchInput = document.getElementById('file-search');
    const fileTree = document.getElementById('file-tree');
    const clearBtn = document.getElementById('search-clear');
    const searchContainer = searchInput ? searchInput.closest('.sidebar-search') : null;
    if (!searchInput || !fileTree) return;

    // Store pre-search expanded state so we can restore it
    var preSearchExpanded = null;

    // Create no-results message
    var noResults = document.createElement('div');
    noResults.className = 'search-no-results';
    noResults.textContent = 'No matching files';
    noResults.style.display = 'none';
    fileTree.appendChild(noResults);

    function updateClearButton() {
      if (searchContainer) {
        searchContainer.classList.toggle('has-query', searchInput.value.trim() !== '');
      }
    }

    // Clear button
    if (clearBtn) {
      clearBtn.addEventListener('click', function() {
        searchInput.value = '';
        sessionStorage.removeItem('gcovr-search');
        updateClearButton();
        performSearch('');
        searchInput.focus();
      });
    }

    var debounceTimer = null;
    searchInput.addEventListener('input', function() {
      updateClearButton();
      clearTimeout(debounceTimer);
      debounceTimer = setTimeout(function() {
        var val = searchInput.value;
        if (val.trim() !== '') {
          sessionStorage.setItem('gcovr-search', val);
        } else {
          sessionStorage.removeItem('gcovr-search');
        }
        performSearch(val);
      }, 150);
    });

    // Restore search state from sessionStorage on page load (synchronous
    // since initFileTree has already built the tree before initSearch runs)
    var savedSearch = sessionStorage.getItem('gcovr-search');
    if (savedSearch) {
      searchInput.value = savedSearch;
      updateClearButton();
      performSearch(savedSearch);
    }

    function performSearch(value) {
      var query = value.toLowerCase().trim();
      var allItems = fileTree.querySelectorAll('.tree-item');

      // Clear all highlights
      fileTree.querySelectorAll('.search-highlight').forEach(function(mark) {
        var parent = mark.parentNode;
        parent.replaceChild(document.createTextNode(mark.textContent), mark);
        parent.normalize();
      });

      // If query is empty, restore original state
      if (query === '') {
        noResults.style.display = 'none';
        allItems.forEach(function(item) {
          item.style.display = '';
          item.classList.remove('search-match');
        });
        // Restore pre-search expanded state
        if (preSearchExpanded !== null) {
          allItems.forEach(function(item) {
            var path = item.getAttribute('data-tree-path');
            var toggle = item.querySelector(':scope > .tree-item-header > .tree-folder-toggle');
            if (toggle) {
              if (preSearchExpanded.indexOf(path) >= 0) {
                item.classList.add('expanded');
                toggle.textContent = '\u2212';
              } else {
                item.classList.remove('expanded');
                toggle.textContent = '+';
              }
            }
          });
          preSearchExpanded = null;
        }
        return;
      }

      // Save expanded state before first search
      if (preSearchExpanded === null) {
        preSearchExpanded = [];
        allItems.forEach(function(item) {
          if (item.classList.contains('expanded')) {
            preSearchExpanded.push(item.getAttribute('data-tree-path'));
          }
        });
      }

      // Determine which items match (check full path and display name)
      var matchSet = new Set();

      allItems.forEach(function(item) {
        var path = (item.getAttribute('data-tree-path') || '').toLowerCase();
        var label = item.querySelector(':scope > .tree-item-header > .tree-label');
        var text = label ? label.textContent.toLowerCase() : '';
        if (path.includes(query) || text.includes(query)) {
          matchSet.add(item);
        }
      });

      // Also mark all ancestor items of matches as visible
      var visibleSet = new Set(matchSet);
      matchSet.forEach(function(item) {
        var parent = item.parentElement;
        while (parent && parent !== fileTree) {
          if (parent.classList && parent.classList.contains('tree-item')) {
            visibleSet.add(parent);
          }
          parent = parent.parentElement;
        }
      });

      // Apply visibility, expand parents of matches, highlight text
      var anyVisible = false;
      allItems.forEach(function(item) {
        var isVisible = visibleSet.has(item);
        item.style.display = isVisible ? '' : 'none';
        item.classList.toggle('search-match', matchSet.has(item));

        if (isVisible) {
          anyVisible = true;
          // Auto-expand folders that contain matches
          var toggle = item.querySelector(':scope > .tree-item-header > .tree-folder-toggle');
          if (toggle && visibleSet.has(item) && !matchSet.has(item) || (toggle && matchSet.has(item) && item.classList.contains('is-folder'))) {
            item.classList.add('expanded');
            toggle.textContent = '\u2212';
          }
        }

        // Highlight matched text in label
        if (matchSet.has(item)) {
          var label = item.querySelector(':scope > .tree-item-header > .tree-label');
          if (label) {
            highlightText(label, query);
          }
        }
      });

      noResults.style.display = anyVisible ? 'none' : '';
    }

    function highlightText(container, query) {
      // Walk text nodes inside the label (may be inside an <a> tag)
      var walker = document.createTreeWalker(container, NodeFilter.SHOW_TEXT, null, false);
      var textNodes = [];
      while (walker.nextNode()) {
        textNodes.push(walker.currentNode);
      }
      textNodes.forEach(function(node) {
        var text = node.textContent;
        var lowerText = text.toLowerCase();
        var idx = lowerText.indexOf(query);
        if (idx === -1) return;

        var frag = document.createDocumentFragment();
        var lastIdx = 0;
        while (idx !== -1) {
          if (idx > lastIdx) {
            frag.appendChild(document.createTextNode(text.substring(lastIdx, idx)));
          }
          var mark = document.createElement('mark');
          mark.className = 'search-highlight';
          mark.textContent = text.substring(idx, idx + query.length);
          frag.appendChild(mark);
          lastIdx = idx + query.length;
          idx = lowerText.indexOf(query, lastIdx);
        }
        if (lastIdx < text.length) {
          frag.appendChild(document.createTextNode(text.substring(lastIdx)));
        }
        node.parentNode.replaceChild(frag, node);
      });
    }
  }

  // ===========================================
  // Progressive Function Row Rendering
  // ===========================================

  function initFunctionRows() {
    var dataEl = document.getElementById('functions-data');
    if (!dataEl) return;

    var config = window.__functionsPageConfig || {};
    var data = JSON.parse(dataEl.textContent);
    var container = document.querySelector('.functions-body');
    var loadingEl = document.getElementById('functions-loading');
    var showBranches = config.showBranches;
    var showConditions = config.showConditions;
    var showDecisions = config.showDecisions;
    var showCalls = config.showCalls;
    var singlePage = config.singlePage;
    var currentFile = config.htmlFilename || '';

    if (data.length === 0) {
      if (loadingEl) loadingEl.remove();
      return;
    }

    // --- Virtual scrolling setup ---
    var ROW_HEIGHT = 52;
    var BUFFER = 10;
    var visibleCount = Math.max(30, Math.ceil(container.clientHeight / ROW_HEIGHT) + BUFFER * 2);
    var viewport, visibleEl;
    var lastStartIdx = -1;

    window.addEventListener('resize', function() {
      visibleCount = Math.max(30, Math.ceil(container.clientHeight / ROW_HEIGHT) + BUFFER * 2);
      lastStartIdx = -1;
      renderVisible();
    });

    function buildHref(entry) {
      if (singlePage) return '#' + entry.html_filename + '|l' + entry.line;
      if (currentFile !== entry.html_filename) return entry.html_filename + '#l' + entry.line;
      return '#l' + entry.line;
    }

    function entryKey(entry) {
      return entry.name + '|' + entry.filename + ':' + entry.line;
    }

    function el(tag, cls, text) {
      var node = document.createElement(tag);
      if (cls) node.className = cls;
      if (text !== undefined) node.textContent = text;
      return node;
    }

    function createRow(entry) {
      var row = el('div', 'function-row');
      if (highlightKey && entryKey(entry) === highlightKey) {
        row.classList.add('function-row-visited');
      }

      // col-function
      var colFn = el('div', 'col-function');
      var a = document.createElement('a');
      a.href = buildHref(entry);
      a.appendChild(el('span', 'function-name', entry.name));
      a.appendChild(el('span', 'function-location', entry.filename + ':' + entry.line));
      colFn.appendChild(a);
      row.appendChild(colFn);

      // col-calls
      var colCalls = el('div', 'col-calls');
      var callSpan;
      if (entry.excluded) {
        callSpan = el('span', 'excluded', 'excluded');
      } else if (entry.execution_count === 0) {
        callSpan = el('span', 'not-called', 'not called');
      } else {
        callSpan = el('span', 'called', entry.execution_count + 'x');
      }
      colCalls.appendChild(callSpan);
      row.appendChild(colCalls);

      // col-lines
      row.appendChild(el('div', 'col-lines', entry.line_coverage + '%'));

      // col-branches (optional)
      if (showBranches) {
        row.appendChild(el('div', 'col-branches', entry.branch_coverage + '%'));
      }

      // col-conditions (optional)
      if (showConditions) {
        row.appendChild(el('div', 'col-conditions', entry.condition_coverage + '%'));
      }

      // col-decisions (optional)
      if (showDecisions) {
        row.appendChild(el('div', 'col-decisions', entry.decision_coverage + '%'));
      }

      // col-calls (optional)
      if (showCalls) {
        row.appendChild(el('div', 'col-calls', entry.call_coverage + '%'));
      }

      return row;
    }

    function setupVirtualScroll() {
      if (loadingEl) loadingEl.remove();

      viewport = document.createElement('div');
      viewport.className = 'functions-viewport';
      viewport.style.height = (data.length * ROW_HEIGHT) + 'px';

      visibleEl = document.createElement('div');
      visibleEl.className = 'functions-visible';

      viewport.appendChild(visibleEl);
      container.appendChild(viewport);
    }

    function renderVisible() {
      var scrollTop = container.scrollTop;
      var startIdx = Math.max(0, Math.floor(scrollTop / ROW_HEIGHT) - BUFFER);
      var endIdx = Math.min(data.length, startIdx + visibleCount + BUFFER);

      // Skip re-render if the window hasn't shifted
      if (startIdx === lastStartIdx) return;
      lastStartIdx = startIdx;

      visibleEl.style.top = (startIdx * ROW_HEIGHT) + 'px';

      var frag = document.createDocumentFragment();
      for (var i = startIdx; i < endIdx; i++) {
        frag.appendChild(createRow(data[i]));
      }
      visibleEl.replaceChildren(frag);
    }

    // --- Scroll listener (rAF-throttled) ---
    var ticking = false;
    container.addEventListener('scroll', function() {
      if (!ticking) {
        requestAnimationFrame(function() { renderVisible(); ticking = false; });
        ticking = true;
      }
    });

    // --- Save state on navigation for back-button restore ---
    var highlightKey = null;
    container.addEventListener('click', function(e) {
      var row = e.target.closest('.function-row');
      if (!row) return;
      var link = row.querySelector('a');
      if (!link) return;
      var nameEl = row.querySelector('.function-name');
      var locEl = row.querySelector('.function-location');
      if (nameEl && locEl) {
        sessionStorage.setItem('gcovr-functions-clicked', nameEl.textContent + '|' + locEl.textContent);
      }
      sessionStorage.setItem('gcovr-functions-scrollTop', String(container.scrollTop));
    });

    // --- Data-level sorting ---
    function sortData(key, ascending) {
      data.sort(function(a, b) {
        var aVal, bVal;
        switch (key) {
          case 'name': aVal = a.name; bVal = b.name; break;
          case 'calls': aVal = a.excluded ? -1 : a.execution_count; bVal = b.excluded ? -1 : b.execution_count; break;
          case 'lines': aVal = parseFloat(a.line_coverage) || 0; bVal = parseFloat(b.line_coverage) || 0; break;
          case 'branches': aVal = parseFloat(a.branch_coverage) || 0; bVal = parseFloat(b.branch_coverage) || 0; break;
          case 'conditions': aVal = parseFloat(a.condition_coverage) || 0; bVal = parseFloat(b.condition_coverage) || 0; break;
          default: aVal = a.name; bVal = b.name;
        }
        if (typeof aVal === 'string' && typeof bVal === 'string') {
          return ascending ? aVal.localeCompare(bVal) : bVal.localeCompare(aVal);
        }
        return ascending ? aVal - bVal : bVal - aVal;
      });
      lastStartIdx = -1; // force re-render
      viewport.style.height = (data.length * ROW_HEIGHT) + 'px';
      renderVisible();
    }

    // Intercept sort clicks on functions-header before initSorting runs
    var funcHeaders = document.querySelectorAll('.functions-header .sortable');
    funcHeaders.forEach(function(header) {
      header.addEventListener('click', function(e) {
        e.stopPropagation();
        var sortKey = this.dataset.sort;
        var isAscending = this.classList.contains('sorted-ascending');

        // Update header classes
        funcHeaders.forEach(function(h) {
          h.classList.remove('sorted-ascending', 'sorted-descending');
        });
        this.classList.add(isAscending ? 'sorted-descending' : 'sorted-ascending');

        sortData(sortKey, !isAscending);
      }, true); // capture phase to beat initSorting
    });

    // --- Restore saved state (scroll + highlight) ---
    function restoreSavedState() {
      var saved = sessionStorage.getItem('gcovr-functions-clicked');
      if (saved !== null) {
        sessionStorage.removeItem('gcovr-functions-clicked');
        highlightKey = saved;
      }
      var scroll = sessionStorage.getItem('gcovr-functions-scrollTop');
      if (scroll !== null) {
        sessionStorage.removeItem('gcovr-functions-scrollTop');
        container.scrollTop = parseInt(scroll, 10);
      }
      if (saved !== null || scroll !== null) {
        lastStartIdx = -1;
        renderVisible();
      }
    }

    // --- Initialize ---
    data.sort(function(a, b) { return a.name.localeCompare(b.name); });

    setupVirtualScroll();
    renderVisible();
    restoreSavedState();

    // Also restore on bfcache navigation (browser Back button)
    window.addEventListener('pageshow', function(e) {
      if (e.persisted) restoreSavedState();
    });

    // Mark functions page so initSorting can skip it
    container.dataset.virtualScroll = 'true';
  }



  // ===========================================
  // Sorting
  // ===========================================

  function initSorting() {
    var headerSets = [
      {
        selector: '.file-list-header .sortable, .functions-header .sortable',
        getContainer: function() {
          return document.getElementById('file-list') || document.querySelector('.functions-body');
        },
        defaultSort: { key: 'filename', ascending: true }
      },
      {
        selector: '.source-function-header .sortable',
        getContainer: function() {
          return document.querySelector('.source-functions-list');
        },
        defaultSort: null
      }
    ];

    headerSets.forEach(function(set) {
      var headers = document.querySelectorAll(set.selector);
      if (!headers.length) return;

      headers.forEach(function(header) {
        header.addEventListener('click', function() {
          var sortKey = this.dataset.sort;
          var isAscending = this.classList.contains('sorted-ascending');

          headers.forEach(function(h) {
            h.classList.remove('sorted-ascending', 'sorted-descending');
          });

          this.classList.add(isAscending ? 'sorted-descending' : 'sorted-ascending');

          sortList(set.getContainer(), sortKey, !isAscending);
        });
      });

      if (set.defaultSort) {
        sortList(set.getContainer(), set.defaultSort.key, set.defaultSort.ascending);
      }
    });
  }

  function sortList(container, key, ascending) {
    if (!container) return;
    // Virtual scroll handles its own sorting
    if (container.dataset.virtualScroll) return;

    var headerEl = container.querySelector('.source-function-header, .file-list-header, .functions-header');
    var rows = Array.from(container.children).filter(function(el) { return el !== headerEl; });

    rows.sort(function(a, b) {
      // Directories always come first
      var aIsDir = a.classList.contains('directory');
      var bIsDir = b.classList.contains('directory');
      if (aIsDir && !bIsDir) return -1;
      if (!aIsDir && bIsDir) return 1;

      var aVal = a.dataset[key] || a.querySelector('[data-sort]')?.dataset.sort || '';
      var bVal = b.dataset[key] || b.querySelector('[data-sort]')?.dataset.sort || '';
      if (key == 'filename' && localStorage.getItem('gcovr-view-mode') === 'nested') {
        aVal = aVal.split('/').pop();
        bVal = bVal.split('/').pop();
      }

      // Try to parse as numbers
      var aNum = parseFloat(aVal);
      var bNum = parseFloat(bVal);

      if (!isNaN(aNum) && !isNaN(bNum)) {
        return ascending ? aNum - bNum : bNum - aNum;
      }

      // String comparison
      return ascending ? aVal.localeCompare(bVal) : bVal.localeCompare(aVal);
    });

    rows.forEach(function(row) {
      container.appendChild(row);
    });
  }

  // ===========================================
  // Toggle Buttons (Coverage Lines)
  // ===========================================

  function initToggleButtons() {
    const buttons = document.querySelectorAll('.button_toggle_coveredLine, .button_toggle_uncoveredLine, .button_toggle_partialCoveredLine, .button_toggle_excludedLine');

    buttons.forEach(function(button) {
      var lineClass = button.value;
      if (!document.querySelector('.' + lineClass)) {
        button.disabled = true;
        button.classList.remove('show_' + lineClass);
        return;
      }
      button.addEventListener('click', function() {
        const lineClass = this.value;
        const showClass = 'show_' + lineClass;

        // Toggle the button state
        this.classList.toggle(showClass);

        // Toggle visibility of lines
        const lines = document.querySelectorAll('.' + lineClass);
        lines.forEach(function(line) {
          line.classList.toggle(showClass);
        });
        document.dispatchEvent(new CustomEvent('coverage-toggled'));
      });
    });

    // Also handle simpler toggle buttons
    const simpleToggles = document.querySelectorAll('.btn-toggle');
    simpleToggles.forEach(function(button) {
      button.addEventListener('click', function() {
        // Use data attribute to get line class (persists after toggle)
        const lineClass = this.dataset.lineClass;
        if (!lineClass) return;

        const showClass = 'show_' + lineClass;
        this.classList.toggle(showClass);
        const lines = document.querySelectorAll('.' + lineClass);
        lines.forEach(function(line) {
          line.classList.toggle(showClass);
        });
        document.dispatchEvent(new CustomEvent('coverage-toggled'));
      });
    });
  }

  // ===========================================
  // Coverage Navigation (prev/next uncovered)
  // ===========================================

  function initCoverageNav() {
    var prevBtn = document.getElementById('nav-prev');
    var nextBtn = document.getElementById('nav-next');
    var counter = document.getElementById('nav-counter');

    if (!prevBtn || !nextBtn || !counter) return;

    var gapLines = [];
    var currentIndex = -1;

    function collectGapLines() {
      var uncovered = document.querySelectorAll('tr.uncoveredLine.show_uncoveredLine');
      var partial = document.querySelectorAll('tr.partialCoveredLine.show_partialCoveredLine');
      var merged = [];
      var i;
      for (i = 0; i < uncovered.length; i++) merged.push(uncovered[i]);
      for (i = 0; i < partial.length; i++) merged.push(partial[i]);
      // Sort by DOM order
      merged.sort(function(a, b) {
        var pos = a.compareDocumentPosition(b);
        if (pos & Node.DOCUMENT_POSITION_FOLLOWING) return -1;
        if (pos & Node.DOCUMENT_POSITION_PRECEDING) return 1;
        return 0;
      });
      gapLines = merged;
      currentIndex = -1;
      updateCounter();
    }

    function updateCounter() {
      if (gapLines.length === 0) {
        counter.textContent = 'All lines covered';
        prevBtn.disabled = true;
        nextBtn.disabled = true;
      } else {
        var display = currentIndex >= 0 ? (currentIndex + 1) : 0;
        counter.textContent = display + ' / ' + gapLines.length;
        prevBtn.disabled = false;
        nextBtn.disabled = false;
      }
    }

    function navigateTo(index) {
      if (gapLines.length === 0) return;
      // Remove previous highlight
      var prev = document.querySelector('tr.source-line.nav-highlight');
      if (prev) prev.classList.remove('nav-highlight');

      currentIndex = index;
      var row = gapLines[currentIndex];
      row.scrollIntoView({ block: 'center', behavior: 'instant' });
      row.classList.add('nav-highlight');
      setTimeout(function() {
        row.classList.remove('nav-highlight');
      }, 1500);
      updateCounter();
    }

    function nextGap() {
      if (gapLines.length === 0) return;
      var next = currentIndex + 1;
      if (next >= gapLines.length) next = 0;
      navigateTo(next);
    }

    function prevGap() {
      if (gapLines.length === 0) return;
      var prev = currentIndex - 1;
      if (prev < 0) prev = gapLines.length - 1;
      navigateTo(prev);
    }

    prevBtn.addEventListener('click', prevGap);
    nextBtn.addEventListener('click', nextGap);

    document.addEventListener('keydown', function(e) {
      var tag = (e.target.tagName || '').toLowerCase();
      if (tag === 'input' || tag === 'textarea' || e.target.isContentEditable) return;
      if (e.key === 'n') nextGap();
      if (e.key === 'p') prevGap();
    });

    document.addEventListener('coverage-toggled', function() {
      collectGapLines();
    });

    collectGapLines();
  }

  // ===========================================
  // View Toggle (Nested / Flat)
  // ===========================================

  function initViewToggle() {
    var toggleContainer = document.getElementById('view-toggle');
    var fileList = document.getElementById('file-list');
    var appContainer = document.querySelector('.app-container');

    if (!toggleContainer) return;

    // Always show the toggle
    toggleContainer.style.display = '';

    var buttons = toggleContainer.querySelectorAll('.view-btn');
    var savedView = localStorage.getItem('gcovr-view-mode');

    function setActiveButton(view) {
      buttons.forEach(function(btn) {
        btn.classList.toggle('active', btn.dataset.view === view);
      });
    }

    // On non-directory pages (file/source views), still respect flat mode for sidebar
    if (!fileList) {
      if (appContainer && savedView === 'flat') {
        appContainer.classList.add('flat-mode');
        setActiveButton('flat');
      }

      // Allow toggling view mode from source pages
      buttons.forEach(function(btn) {
        btn.addEventListener('click', function() {
          var view = this.dataset.view;
          localStorage.setItem('gcovr-view-mode', view);
          setActiveButton(view);
          if (appContainer) {
            if (view === 'flat') {
              appContainer.classList.add('flat-mode');
            } else {
              appContainer.classList.remove('flat-mode');
              document.documentElement.classList.remove('early-flat-mode');
            }
          }
        });
      });
      return;
    }

    var originalNodes = null; // stash for restoring nested view

    function collectFlatFiles(nodes, parentPath) {
      var results = [];
      for (var i = 0; i < nodes.length; i++) {
        var node = nodes[i];
        var cleanedName = node.name;
        // Remove leading ./ or ../
        while (cleanedName.indexOf('./') === 0 || cleanedName.indexOf('../') === 0) {
          cleanedName = cleanedName.indexOf('./') === 0 ? cleanedName.substring(2) : cleanedName.substring(3);
        }
        var fullPath = parentPath ? (parentPath + '/' + cleanedName) : cleanedName;

        if (node.isDirectory && node.children && node.children.length > 0) {
          results = results.concat(collectFlatFiles(node.children, fullPath));
        } else if (!node.isDirectory) {
          var copy = {};
          for (var key in node) {
            if (node.hasOwnProperty(key)) copy[key] = node[key];
          }
          copy.fullPath = fullPath;
          results.push(copy);
        }
      }
      return results;
    }

    function buildFlatRow(file) {
      var row = document.createElement('div');
      row.className = 'file-row file';
      row.setAttribute('data-filename', file.fullPath);
      row.setAttribute('data-coverage', file.coverage || '0');
      row.setAttribute('data-lines', file.linesTotal || '');
      row.setAttribute('data-functions', file.functionsCoverage || '');
      row.setAttribute('data-branches', file.branchesCoverage || '');

      // Col name
      var colName = document.createElement('div');
      colName.className = 'col-name';

      var icon = document.createElement('span');
      icon.className = 'file-icon';
      icon.innerHTML = '<svg viewBox="0 0 16 16" width="16" height="16"><path fill="currentColor" d="M3.75 1.5a.25.25 0 00-.25.25v12.5c0 .138.112.25.25.25h9.5a.25.25 0 00.25-.25V6h-2.75A1.75 1.75 0 019 4.25V1.5H3.75zm6.75.062V4.25c0 .138.112.25.25.25h2.688a.252.252 0 00-.011-.013l-2.914-2.914a.272.272 0 00-.013-.011zM2 1.75C2 .784 2.784 0 3.75 0h6.586c.464 0 .909.184 1.237.513l2.914 2.914c.329.328.513.773.513 1.237v9.586A1.75 1.75 0 0113.25 16h-9.5A1.75 1.75 0 012 14.25V1.75z"></path></svg>';
      colName.appendChild(icon);

      if (file.link) {
        var a = document.createElement('a');
        a.href = file.link;
        a.textContent = file.fullPath;
        a.title = file.fullPath;
        colName.appendChild(a);
      } else {
        var span = document.createElement('span');
        span.className = 'no-link';
        span.textContent = file.fullPath;
        span.title = file.fullPath;
        colName.appendChild(span);
      }
      row.appendChild(colName);

      // Col coverage
      var colCov = document.createElement('div');
      colCov.className = 'col-coverage';

      var barContainer = document.createElement('div');
      barContainer.className = 'coverage-bar-container';
      var bar = document.createElement('div');
      var linesCov = file.linesCoverage || '';
      var linesClass = file.linesClass || file.coverageClass || '';
      bar.className = 'coverage-bar ' + linesClass;
      bar.style.width = (linesCov && linesCov !== '-') ? linesCov + '%' : '0%';
      barContainer.appendChild(bar);
      colCov.appendChild(barContainer);

      var pct = document.createElement('span');
      pct.className = 'coverage-percent ' + linesClass;
      pct.textContent = (linesCov && linesCov !== '-') ? linesCov + '%' : '-';
      colCov.appendChild(pct);
      row.appendChild(colCov);

      // Col lines
      var colLines = document.createElement('div');
      colLines.className = 'col-lines';
      var execSpan = document.createElement('span');
      execSpan.className = 'stat-value';
      execSpan.textContent = file.linesExec || '';
      colLines.appendChild(execSpan);
      var sep = document.createElement('span');
      sep.className = 'stat-separator';
      sep.textContent = '/';
      colLines.appendChild(sep);
      var totalSpan = document.createElement('span');
      totalSpan.className = 'stat-total';
      totalSpan.textContent = file.linesTotal || '';
      colLines.appendChild(totalSpan);
      row.appendChild(colLines);

      // Col functions (check if container has the column)
      var container = fileList.closest('.file-list-container');
      var hasFunctions = !container || !container.classList.contains('no-functions');
      var hasBranches = !container || !container.classList.contains('no-branches');
      var hasConditions = !container || !container.classList.contains('no-conditions');
      var hasDecision = !container || !container.classList.contains('no-decisions');
      var hasCalls = !container || !container.classList.contains('no-calls');

      if (hasFunctions) {
        var colFunc = document.createElement('div');
        colFunc.className = 'col-functions';
        var funcVal = document.createElement('span');
        var funcCov = file.functionsCoverage || '';
        var funcClass = file.functionsClass || '';
        funcVal.className = 'stat-value ' + funcClass;
        funcVal.textContent = (funcCov && funcCov !== '-') ? funcCov + '%' : '-';
        colFunc.appendChild(funcVal);
        row.appendChild(colFunc);
      }

      if (hasBranches) {
        var colBr = document.createElement('div');
        colBr.className = 'col-branches';
        var brVal = document.createElement('span');
        var brCov = file.branchesCoverage || '';
        var brClass = file.branchesClass || '';
        brVal.className = 'stat-value ' + brClass;
        brVal.textContent = (brCov && brCov !== '-') ? brCov + '%' : '-';
        colBr.appendChild(brVal);
        row.appendChild(colBr);
      }

      if (hasConditions) {
        var colCond = document.createElement('div');
        colCond.className = 'col-conditions';
        var condVal = document.createElement('span');
        var condCov = file.conditionsCoverage || '';
        var condClass = file.conditionsClass || '';
        condVal.className = 'stat-value ' + condClass;
        condVal.textContent = (condCov && condCov !== '-') ? condCov + '%' : '-';
        colCond.appendChild(condVal);
        row.appendChild(colCond);
      }

      if (hasDecision) {
        var colDec = document.createElement('div');
        colDec.className = 'col-decision';
        var decVal = document.createElement('span');
        var decCov = file.decisionCoverage || '';
        var decClass = file.decisionClass || '';
        decVal.className = 'stat-value ' + decClass;
        decVal.textContent = (decCov && decCov !== '-') ? decCov + '%' : '-';
        colDec.appendChild(decVal);
        row.appendChild(colDec);
      }

      if (hasCalls) {
        var colCalls = document.createElement('div');
        colCalls.className = 'col-calls';
        var callsVal = document.createElement('span');
        var callsCov = file.callsCoverage || '';
        var callsClass = file.callsClass || '';
        callsVal.className = 'stat-value ' + callsClass;
        callsVal.textContent = (callsCov && callsCov !== '-') ? callsCov + '%' : '-';
        colCalls.appendChild(callsVal);
        row.appendChild(colCalls);
      }


      return row;
    }

    function switchToFlat() {
      if (!window.GCOVR_TREE_DATA) return;

      // Stash original DOM nodes
      if (originalNodes === null) {
        originalNodes = document.createDocumentFragment();
        while (fileList.firstChild) {
          originalNodes.appendChild(fileList.firstChild);
        }
      }

      var flatFiles = collectFlatFiles(window.GCOVR_TREE_DATA, '');

      // Sort by coverage ascending (matching default)
      flatFiles.sort(function(a, b) {
        var aVal = parseFloat(a.coverage) || 0;
        var bVal = parseFloat(b.coverage) || 0;
        return aVal - bVal;
      });

      while (fileList.firstChild) {
        fileList.removeChild(fileList.firstChild);
      }
      for (var i = 0; i < flatFiles.length; i++) {
        fileList.appendChild(buildFlatRow(flatFiles[i]));
      }

      if (appContainer) appContainer.classList.add('flat-mode');
      setActiveButton('flat');
      localStorage.setItem('gcovr-view-mode', 'flat');
    }

    function switchToNested() {
      if (originalNodes !== null) {
        while (fileList.firstChild) {
          fileList.removeChild(fileList.firstChild);
        }
        fileList.appendChild(originalNodes);
        originalNodes = null;
      }
      if (appContainer) appContainer.classList.remove('flat-mode');
      document.documentElement.classList.remove('early-flat-mode');
      setActiveButton('nested');
      localStorage.setItem('gcovr-view-mode', 'nested');

      // Re-run sorting to maintain state
      sortList(document.getElementById('file-list') || document.querySelector('.functions-body'), 'filename', true);
    }

    buttons.forEach(function(btn) {
      btn.addEventListener('click', function() {
        var view = this.dataset.view;
        if (view === 'flat') {
          switchToFlat();
        } else {
          switchToNested();
        }
      });
    });

    // Apply saved preference on load
    if (savedView === 'flat') {
      // Defer to ensure tree data is loaded
      setTimeout(function() {
        switchToFlat();
      }, 0);
    }
  }

  // ===========================================
  // Settings Dropdown (mobile gear icon)
  // ===========================================

  function initSettingsDropdown() {
    var btn = document.getElementById('settings-btn');
    var dropdown = document.getElementById('settings-dropdown');
    var header = document.querySelector('.main-header');
    if (!btn || !dropdown || !header) return;

    var viewToggle = document.getElementById('view-toggle');
    var themeToggle = document.getElementById('theme-toggle');
    var isMobile = false;

    // Reference node: settings-btn, so we can insert before it when moving back
    function moveToDropdown() {
      if (viewToggle && viewToggle.parentNode !== dropdown) {
        dropdown.appendChild(viewToggle);
      }
      if (themeToggle && themeToggle.parentNode !== dropdown) {
        dropdown.appendChild(themeToggle);
      }
    }

    function moveToHeader() {
      // Insert before settings-btn so they appear in original order
      if (viewToggle && viewToggle.parentNode !== header) {
        header.insertBefore(viewToggle, btn);
      }
      if (themeToggle && themeToggle.parentNode !== header) {
        header.insertBefore(themeToggle, btn);
      }
    }

    function checkBreakpoint() {
      var nowMobile = window.innerWidth <= 1024;
      if (nowMobile === isMobile) return;
      isMobile = nowMobile;
      if (isMobile) {
        moveToDropdown();
      } else {
        dropdown.classList.remove('open');
        moveToHeader();
      }
    }

    // Toggle dropdown on button click
    btn.addEventListener('click', function(e) {
      e.stopPropagation();
      dropdown.classList.toggle('open');
    });

    // Close on outside click
    document.addEventListener('click', function(e) {
      if (!dropdown.contains(e.target) && e.target !== btn) {
        dropdown.classList.remove('open');
      }
    });

    // Close on Escape
    document.addEventListener('keydown', function(e) {
      if (e.key === 'Escape') {
        dropdown.classList.remove('open');
      }
    });

    // Respond to resize
    window.addEventListener('resize', checkBreakpoint);

    // Initial check
    checkBreakpoint();
  }

  // ===========================================
  // Popup Resize (only when overflowing)
  // ===========================================

  function initPopupResize() {
    var details = document.querySelectorAll('.branch-details, .condition-details, .decision-details, .call-details');
    if (details.length === 0) return;

    details.forEach(function(det) {
      det.addEventListener('toggle', function() {
        if (!det.open) return;
        var popup = det.querySelector('.branch-popup, .condition-popup, .decision-popup, .call-popup');
        if (!popup) return;
        // Check after render if content overflows
        requestAnimationFrame(function() {
          if (popup.scrollHeight > popup.clientHeight) {
            popup.classList.add('is-overflowing');
          } else {
            popup.classList.remove('is-overflowing');
          }
        });
      });
    });
  }

  // ===========================================
  // Nav Override (prev/next follows tree order)
  // ===========================================

  function initNavOverride() {
    if (!window.GCOVR_TREE_DATA) return;

    var navPrev = document.querySelectorAll('.nav-prev');
    var navNext = document.querySelectorAll('.nav-next');
    if (navPrev.length === 0 && navNext.length === 0) return;

    // DFS-flatten tree to collect file links in sidebar order
    function collectLinks(nodes) {
      var links = [];
      for (var i = 0; i < nodes.length; i++) {
        var node = nodes[i];
        if (node.isDirectory && node.children && node.children.length > 0) {
          links = links.concat(collectLinks(node.children));
        } else if (!node.isDirectory && node.link) {
          links.push(node.link);
        }
      }
      return links;
    }

    var fileLinks = collectLinks(window.GCOVR_TREE_DATA);
    if (fileLinks.length === 0) return;

    var currentPage = window.location.pathname.split('/').pop() || 'index.html';
    var idx = fileLinks.indexOf(currentPage);
    if (idx === -1) return;

    var prev = idx > 0 ? fileLinks[idx - 1] : null;
    var next = idx < fileLinks.length - 1 ? fileLinks[idx + 1] : null;

    function updateNavLinks(els, href) {
      for (var i = 0; i < els.length; i++) {
        var el = els[i];
        if (href) {
          // Enable: ensure it's an <a> with the correct href
          if (el.tagName === 'A') {
            el.setAttribute('href', href);
          } else {
            // Replace disabled <span> with an <a>
            var a = document.createElement('a');
            a.className = el.className.replace(/\bdisabled\b/, '').trim();
            a.href = href;
            a.title = el.title;
            while (el.firstChild) a.appendChild(el.firstChild);
            el.parentNode.replaceChild(a, el);
          }
        } else {
          // Disable: ensure it's a <span> with disabled class
          if (el.tagName === 'A') {
            var span = document.createElement('span');
            span.className = el.className + ' disabled';
            span.title = el.title;
            while (el.firstChild) span.appendChild(el.firstChild);
            el.parentNode.replaceChild(span, el);
          } else {
            el.classList.add('disabled');
          }
        }
      }
    }

    updateNavLinks(navPrev, prev);
    updateNavLinks(navNext, next);
  }

  // ===========================================
  // TLA Navigation (HIT/MIS/PAR links)
  // ===========================================

  function initTlaNavigation() {
    var rows = document.querySelectorAll('.source-line');
    if (rows.length === 0) return;

    // Classify each row by coverage type
    var COV_CLASSES = ['coveredLine', 'uncoveredLine', 'partialCoveredLine'];
    var LABELS = { coveredLine: 'HIT', uncoveredLine: 'MIS', partialCoveredLine: 'PAR' };
    var CSS_CLASSES = { coveredLine: 'tla-hit', uncoveredLine: 'tla-mis', partialCoveredLine: 'tla-par' };

    // Build list of groups: contiguous runs of the same coverage class
    var groups = []; // { type, firstRow }
    var prevType = null;

    for (var i = 0; i < rows.length; i++) {
      var row = rows[i];
      var type = null;
      for (var j = 0; j < COV_CLASSES.length; j++) {
        if (row.classList.contains(COV_CLASSES[j])) {
          type = COV_CLASSES[j];
          break;
        }
      }
      if (type === null) {
        prevType = null;
        continue;
      }
      if (type !== prevType) {
        groups.push({ type: type, firstRow: row });
        prevType = type;
      }
    }

    if (groups.length === 0) return;

    // Determine the anchor prefix used in this page
    var sampleAnchor = rows[0].querySelector('.col-lineno a');
    var anchorPrefix = '';
    if (sampleAnchor) {
      var id = sampleAnchor.id;
      var idx = id.indexOf('l');
      if (idx > 0) {
        anchorPrefix = id.substring(0, idx);
      }
    }

    // For each group, find the line number from its first row
    function getLineNo(row) {
      var a = row.querySelector('.col-lineno a');
      return a ? a.textContent.trim() : '';
    }

    // Build per-type lists for wrap-around linking
    var byType = {};
    for (var i = 0; i < groups.length; i++) {
      var g = groups[i];
      if (!byType[g.type]) byType[g.type] = [];
      byType[g.type].push(i);
    }

    // For each group, compute next group index of same type
    var nextGroupIdx = new Array(groups.length);
    for (var type in byType) {
      var indices = byType[type];
      for (var k = 0; k < indices.length; k++) {
        var nextK = (k + 1) % indices.length;
        nextGroupIdx[indices[k]] = indices[nextK];
      }
    }

    // Inject TLA links
    for (var i = 0; i < groups.length; i++) {
      var g = groups[i];
      var cell = g.firstRow.querySelector('.col-tla');
      if (!cell) continue;

      var targetGroup = groups[nextGroupIdx[i]];
      var targetLineNo = getLineNo(targetGroup.firstRow);

      var targetId = anchorPrefix + 'l' + targetLineNo;

      var a = document.createElement('a');
      a.className = 'tla-link ' + CSS_CLASSES[g.type];
      a.textContent = LABELS[g.type];
      a.href = '#' + targetId;
      a.addEventListener('click', function(e) {
        var target = document.getElementById(this.getAttribute('href').substring(1));
        if (target) {
          e.preventDefault();
          // Scroll within the source-table-container
          var scrollBox = document.querySelector('.source-table-container');
          var row = target.closest('tr');
          if (scrollBox && row) {
            var thead = scrollBox.querySelector('thead');
            var theadHeight = thead ? thead.offsetHeight : 0;
            scrollBox.scrollTo({ top: row.offsetTop - theadHeight - 8, behavior: 'instant' });
          }
          history.replaceState(null, '', this.getAttribute('href'));
          // Highlight the target row (clear any previous highlight first)
          var prev = document.querySelector('.highlight-target');
          if (prev) prev.classList.remove('highlight-target');
          if (row) row.classList.add('highlight-target');
        }
      });
      cell.appendChild(a);
    }
  }

  // ===========================================
  // Line number click highlight
  // ===========================================

  function initLineHighlight() {
    var clickedFnItem = null;

    function highlightFromHash(scroll) {
      var prev = document.querySelector('.highlight-target');
      if (prev) prev.classList.remove('highlight-target');
      var prevFn = document.querySelector('.source-function-item.selected');
      if (prevFn) prevFn.classList.remove('selected');
      var id = window.location.hash.slice(1);
      if (!id) return;
      var el = document.getElementById(id);
      if (!el) return;
      var fnItem = clickedFnItem || document.querySelector('.source-function-item[href="#' + id + '"]');
      clickedFnItem = null;
      if (fnItem) fnItem.classList.add('selected');
      var row = el.closest('tr');
      if (row) {
        row.classList.add('highlight-target');
        if (scroll) {
          var scrollBox = document.querySelector('.source-table-container');
          if (scrollBox) {
            var thead = scrollBox.querySelector('thead');
            var theadHeight = thead ? thead.offsetHeight : 0;
            scrollBox.scrollTo({ top: row.offsetTop - theadHeight - 8, behavior: 'instant' });
          } else {
            row.scrollIntoView({ block: 'center' });
          }
        }
      }
    }

    // Handle clicks on function list items directly
    var fnList = document.querySelector('.source-functions-list');
    if (fnList) {
      fnList.addEventListener('click', function(e) {
        var item = e.target.closest('.source-function-item');
        if (!item) return;
        e.preventDefault();
        clickedFnItem = item;
        var href = item.getAttribute('href');
        if (href) history.replaceState(null, '', href);
        highlightFromHash(true);
      });
    }

    // Event delegation: single listener on the table container
    var container = document.querySelector('.source-table-container');
    if (container) {
      container.addEventListener('click', function(e) {
        var anchor = e.target.closest('.col-lineno a');
        if (!anchor) return;
        e.preventDefault();
        if (anchor.id) history.replaceState(null, '', '#' + anchor.id);
        highlightFromHash(false);
      });
    }

    // Highlight + scroll on initial load and back/forward navigation
    highlightFromHash(true);
    window.addEventListener('hashchange', function() { highlightFromHash(true); });
  }

  // ===========================================
  // Column Visibility Toggles
  // ===========================================

  function initColumnToggles() {
    var buttons = document.querySelectorAll('.col-toggle');
    if (buttons.length === 0) return;

    var table = document.querySelector('.source-table');
    if (!table) return;

    // Restore saved state
    var hidden = [];
    try {
      var saved = localStorage.getItem('gcovr-hidden-columns');
      if (saved) {
        hidden = JSON.parse(saved);
      } else {
        hidden = ['tla'];
      }
    } catch (e) {}

    // Apply saved hidden columns
    var fnList = document.querySelector('.source-functions-list');
    for (var i = 0; i < hidden.length; i++) {
      table.classList.add('hide-col-' + hidden[i]);
      if (fnList) {
        fnList.classList.add('hide-col-' + hidden[i]);
      }
    }

    // Update button appearance to match state
    buttons.forEach(function(btn) {
      var col = btn.getAttribute('data-col');
      if (hidden.indexOf(col) >= 0) {
        btn.classList.remove('show-col');
      }
    });

    // Handle clicks
    buttons.forEach(function(btn) {
      btn.addEventListener('click', function() {
        var col = this.getAttribute('data-col');
        var hideClass = 'hide-col-' + col;
        var isHidden = table.classList.toggle(hideClass);
        this.classList.toggle('show-col', !isHidden);

        // Sync with function list sidebar
        var fnList = document.querySelector('.source-functions-list');
        if (fnList) {
          fnList.classList.toggle(hideClass, isHidden);
        }

        // Save state
        var current = [];
        var allBtns = document.querySelectorAll('.col-toggle');
        allBtns.forEach(function(b) {
          if (!b.classList.contains('show-col')) {
            current.push(b.getAttribute('data-col'));
          }
        });
        localStorage.setItem('gcovr-hidden-columns', JSON.stringify(current));
      });
    });
  }

  // ===========================================
  // File nav keyboard shortcuts ([ and ])
  // ===========================================

  function initFileNavKeys() {
    var prevLink = document.querySelector('.source-nav-links .nav-prev') || document.querySelector('.nav-links .nav-prev');
    var nextLink = document.querySelector('.source-nav-links .nav-next') || document.querySelector('.nav-links .nav-next');
    if (!prevLink && !nextLink) return;

    document.addEventListener('keydown', function(e) {
      var tag = (e.target.tagName || '').toLowerCase();
      if (tag === 'input' || tag === 'textarea' || e.target.isContentEditable) return;
      // Re-query to pick up any DOM replacements by initNavOverride
      var prev = document.querySelector('.source-nav-links a.nav-prev') || document.querySelector('.nav-links a.nav-prev');
      var next = document.querySelector('.source-nav-links a.nav-next') || document.querySelector('.nav-links a.nav-next');
      if (e.key === '[' && prev) {
        window.location.href = prev.href;
      }
      if (e.key === ']' && next) {
        window.location.href = next.href;
      }
    });
  }

  // ===========================================
  // Enrich file nav tooltips with actual filenames
  // ===========================================

  function initFileNavTooltips() {
    if (!window.GCOVR_TREE_DATA) return;
    var links = document.querySelectorAll('.source-nav-links .nav-prev, .source-nav-links .nav-next, .nav-links .nav-prev, .nav-links .nav-next');
    for (var i = 0; i < links.length; i++) {
      var anchor = links[i];
      var href = anchor.getAttribute('href');
      if (!href || href === '#') continue;
      var filename = href.replace(/^.*\//, '').replace(/#.*$/, '');
      var node = findNodeInTree(window.GCOVR_TREE_DATA, filename);
      if (node && node.name) {
        var direction = anchor.classList.contains('nav-prev') ? 'Previous' : 'Next';
        anchor.title = direction + ': ' + node.name;
      }
    }
  }

  function findNodeInTree(nodes, targetLink) {
    for (var i = 0; i < nodes.length; i++) {
      var node = nodes[i];
      if (node.link === targetLink) return node;
      if (node.children) {
        var found = findNodeInTree(node.children, targetLink);
        if (found) return found;
      }
    }
    return null;
  }

  // ===========================================
  // Prefetch pages on hover for instant nav
  // ===========================================

  function initPrefetch() {
    // Skip for file:// protocol (fetch won't work)
    if (location.protocol === 'file:') return;

    var prefetched = {};

    document.addEventListener('mouseover', function(e) {
      var link = e.target.closest('a[href]');
      if (!link) return;

      var href = link.getAttribute('href');
      // Only prefetch local HTML pages
      if (!href || href.charAt(0) === '#' || href.indexOf('://') !== -1) return;
      if (prefetched[href]) return;

      prefetched[href] = true;
      var prefetchLink = document.createElement('link');
      prefetchLink.rel = 'prefetch';
      prefetchLink.href = href;
      document.head.appendChild(prefetchLink);
    });
  }

  function initFunctionListPersistence() {
    var details = document.querySelector('details.source-functions');
    if (!details) return;

    var key = 'gcovr-fn-list-open';
    if (sessionStorage.getItem(key) === 'true') {
      details.setAttribute('open', '');
    }

    details.addEventListener('toggle', function() {
      sessionStorage.setItem(key, details.open ? 'true' : 'false');
    });
  }

})();

window.GCOVR_TREE_DATA = [
  {
    "branchesClass": "coverage-unknown",
    "branchesCoverage": "-",
    "children": [
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "children": [
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "100.0",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "100.0",
            "linesExec": "3",
            "linesTotal": "3",
            "link": "index.error.hpp.74d61ff866487bf9925130a2c409a1cc.html",
            "name": "error.hpp"
          }
        ],
        "coverage": "100.0",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-high",
        "functionsCoverage": "100.0",
        "isDirectory": true,
        "linesClass": "coverage-high",
        "linesCoverage": "100.0",
        "linesExec": "3",
        "linesTotal": "3",
        "link": "index.impl.4ea3748f69a221375434602a8c320918.html",
        "name": "brotli/impl"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "children": [
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "85.4",
            "coverageClass": "coverage-medium",
            "functionsClass": "coverage-low",
            "functionsCoverage": "65.1",
            "isDirectory": false,
            "linesClass": "coverage-medium",
            "linesCoverage": "85.4",
            "linesExec": "76",
            "linesTotal": "89",
            "link": "index.polystore.hpp.f5c1ce5a35ab885e3f2ebbfc7fe87112.html",
            "name": "polystore.hpp"
          }
        ],
        "coverage": "85.4",
        "coverageClass": "coverage-medium",
        "functionsClass": "coverage-low",
        "functionsCoverage": "65.1",
        "isDirectory": true,
        "linesClass": "coverage-medium",
        "linesCoverage": "85.4",
        "linesExec": "76",
        "linesTotal": "89",
        "link": "index.core.fbfc55a58eb184ca24ed349b7d00e248.html",
        "name": "core"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "children": [
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "100.0",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "100.0",
            "linesExec": "38",
            "linesTotal": "38",
            "link": "index.schema.hpp.7801cf9c34a9e6fbf41820e7cad78064.html",
            "name": "schema.hpp"
          }
        ],
        "coverage": "100.0",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-high",
        "functionsCoverage": "100.0",
        "isDirectory": true,
        "linesClass": "coverage-high",
        "linesCoverage": "100.0",
        "linesExec": "38",
        "linesTotal": "38",
        "link": "index.db.e9a6369db2736656304775dcc253ffb5.html",
        "name": "db"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "children": [
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "children": [
              {
                "branchesClass": "coverage-unknown",
                "branchesCoverage": "-",
                "coverage": "97.1",
                "coverageClass": "coverage-high",
                "functionsClass": "coverage-high",
                "functionsCoverage": "100.0",
                "isDirectory": false,
                "linesClass": "coverage-high",
                "linesCoverage": "97.1",
                "linesExec": "34",
                "linesTotal": "35",
                "link": "index.workspace.hpp.1de37e9405ce9076fdf602d434004f21.html",
                "name": "workspace.hpp"
              }
            ],
            "coverage": "97.1",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": true,
            "linesClass": "coverage-high",
            "linesCoverage": "97.1",
            "linesExec": "34",
            "linesTotal": "35",
            "link": "index.impl.94ab32da329a65aa39b157b4c919231c.html",
            "name": "impl"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "93.8",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "93.8",
            "linesExec": "15",
            "linesTotal": "16",
            "link": "index.circular_buffer.hpp.b04bcefc2808203883e19f860c0c87e6.html",
            "name": "circular_buffer.hpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "87.5",
            "coverageClass": "coverage-medium",
            "functionsClass": "coverage-unknown",
            "functionsCoverage": "-",
            "isDirectory": false,
            "linesClass": "coverage-medium",
            "linesCoverage": "87.5",
            "linesExec": "7",
            "linesTotal": "8",
            "link": "index.except.hpp.ca7969e435d22d435cab6b03f12fc2c7.html",
            "name": "except.hpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "100.0",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "100.0",
            "linesExec": "5",
            "linesTotal": "5",
            "link": "index.file_posix.hpp.c27cdab4431a24365b56b994c4949d8f.html",
            "name": "file_posix.hpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "100.0",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "100.0",
            "linesExec": "4",
            "linesTotal": "4",
            "link": "index.file_stdio.hpp.c6848ec6912bf55eb51ccfd9f0d2277c.html",
            "name": "file_stdio.hpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "91.7",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "91.7",
            "linesExec": "22",
            "linesTotal": "24",
            "link": "index.flat_buffer.hpp.b1e564c4fcc23a76077c93fc313a9912.html",
            "name": "flat_buffer.hpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "100.0",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "100.0",
            "linesExec": "7",
            "linesTotal": "7",
            "link": "index.header.hpp.2498a56ed2badba6ddd4fc4b1f7324bc.html",
            "name": "header.hpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "100.0",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "100.0",
            "linesExec": "2",
            "linesTotal": "2",
            "link": "index.sv.hpp.5627a7db456856b18506814e9df27c8a.html",
            "name": "sv.hpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "100.0",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "100.0",
            "linesExec": "4",
            "linesTotal": "4",
            "link": "index.workspace.hpp.ba401dfafbccd0f162595c9d6087f3c6.html",
            "name": "workspace.hpp"
          }
        ],
        "coverage": "95.2",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-high",
        "functionsCoverage": "100.0",
        "isDirectory": true,
        "linesClass": "coverage-high",
        "linesCoverage": "95.2",
        "linesExec": "100",
        "linesTotal": "105",
        "link": "index.detail.8e9d762a65d1eb172fcd6ec6c43a11bf.html",
        "name": "detail"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "children": [
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "100.0",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "100.0",
            "linesExec": "6",
            "linesTotal": "6",
            "link": "index.error.hpp.9a835d3d0bd33f63af1a6176c3a8096c.html",
            "name": "error.hpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "100.0",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "100.0",
            "linesExec": "77",
            "linesTotal": "77",
            "link": "index.fields_base.hpp.6e962302dca27cfb125ed6b5e7f5999f.html",
            "name": "fields_base.hpp"
          }
        ],
        "coverage": "100.0",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-high",
        "functionsCoverage": "100.0",
        "isDirectory": true,
        "linesClass": "coverage-high",
        "linesCoverage": "100.0",
        "linesExec": "83",
        "linesTotal": "83",
        "link": "index.impl.3e1636b5fba477e4ce59ec182b2eb87a.html",
        "name": "impl"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "children": [
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "98.4",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "90.2",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "98.4",
            "linesExec": "242",
            "linesTotal": "247",
            "link": "index.any_buffer_sink.hpp.19d6ed3c92462b1ffacb437e411fc171.html",
            "name": "any_buffer_sink.hpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "91.6",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-medium",
            "functionsCoverage": "75.0",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "91.6",
            "linesExec": "142",
            "linesTotal": "156",
            "link": "index.any_buffer_source.hpp.80bac19eec2a72eeab4758fff5e9691f.html",
            "name": "any_buffer_source.hpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "98.8",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-medium",
            "functionsCoverage": "78.1",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "98.8",
            "linesExec": "83",
            "linesTotal": "84",
            "link": "index.any_read_source.hpp.4d29d9b2ffbe3785ab4ca4511d751019.html",
            "name": "any_read_source.hpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "97.6",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-medium",
            "functionsCoverage": "83.9",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "97.6",
            "linesExec": "165",
            "linesTotal": "169",
            "link": "index.any_write_sink.hpp.ad6bd305ada88a47bc34b8c5f79b7fbc.html",
            "name": "any_write_sink.hpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "100.0",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "100.0",
            "linesExec": "4",
            "linesTotal": "4",
            "link": "index.pull_from.hpp.27a17bceef2033b2855f08ea8666c3a3.html",
            "name": "pull_from.hpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "100.0",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-medium",
            "functionsCoverage": "75.0",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "100.0",
            "linesExec": "4",
            "linesTotal": "4",
            "link": "index.push_to.hpp.499571c251f11e58ca38116996272202.html",
            "name": "push_to.hpp"
          }
        ],
        "coverage": "96.7",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-medium",
        "functionsCoverage": "83.9",
        "isDirectory": true,
        "linesClass": "coverage-high",
        "linesCoverage": "96.7",
        "linesExec": "640",
        "linesTotal": "664",
        "link": "index.io.947a1948ba74e0796c76e09359891ceb.html",
        "name": "io"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "children": [
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "90.0",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-medium",
            "functionsCoverage": "75.0",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "90.0",
            "linesExec": "36",
            "linesTotal": "40",
            "link": "index.json_sink.hpp.3de23b2c286c57040f41f00b93eeee06.html",
            "name": "json_sink.hpp"
          }
        ],
        "coverage": "90.0",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-medium",
        "functionsCoverage": "75.0",
        "isDirectory": true,
        "linesClass": "coverage-high",
        "linesCoverage": "90.0",
        "linesExec": "36",
        "linesTotal": "40",
        "link": "index.json.7e4a3d11f22eedc211e55bb494e7f37b.html",
        "name": "json"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "children": [
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "children": [
              {
                "branchesClass": "coverage-unknown",
                "branchesCoverage": "-",
                "coverage": "100.0",
                "coverageClass": "coverage-high",
                "functionsClass": "coverage-high",
                "functionsCoverage": "100.0",
                "isDirectory": false,
                "linesClass": "coverage-high",
                "linesCoverage": "100.0",
                "linesExec": "2",
                "linesTotal": "2",
                "link": "index.ws.hpp.0c6225d471096ed1995d6c7371be5025.html",
                "name": "ws.hpp"
              }
            ],
            "coverage": "100.0",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": true,
            "linesClass": "coverage-high",
            "linesCoverage": "100.0",
            "linesExec": "2",
            "linesTotal": "2",
            "link": "index.detail.8f195047b9a754c880976fc887fb2bb6.html",
            "name": "detail"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "children": [
              {
                "branchesClass": "coverage-unknown",
                "branchesCoverage": "-",
                "coverage": "98.2",
                "coverageClass": "coverage-high",
                "functionsClass": "coverage-high",
                "functionsCoverage": "100.0",
                "isDirectory": false,
                "linesClass": "coverage-high",
                "linesCoverage": "98.2",
                "linesExec": "56",
                "linesTotal": "57",
                "link": "index.list_rule.hpp.4db73c16a687ab54baedc5834da5659d.html",
                "name": "list_rule.hpp"
              }
            ],
            "coverage": "98.2",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": true,
            "linesClass": "coverage-high",
            "linesCoverage": "98.2",
            "linesExec": "56",
            "linesTotal": "57",
            "link": "index.impl.b5cd6a3805b960719d4d7f1f76b5733f.html",
            "name": "impl"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "100.0",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "100.0",
            "linesExec": "6",
            "linesTotal": "6",
            "link": "index.list_rule.hpp.8f1b97457a6ea0d239e3b5a914e502b7.html",
            "name": "list_rule.hpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "100.0",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "100.0",
            "linesExec": "12",
            "linesTotal": "12",
            "link": "index.quoted_token_view.hpp.3d4cdcdfe5a7a80f550cf21c66463d13.html",
            "name": "quoted_token_view.hpp"
          }
        ],
        "coverage": "98.7",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-high",
        "functionsCoverage": "100.0",
        "isDirectory": true,
        "linesClass": "coverage-high",
        "linesCoverage": "98.7",
        "linesExec": "76",
        "linesTotal": "77",
        "link": "index.rfc.76176b3e4e934007bad4d9d8c3e720fc.html",
        "name": "rfc"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "children": [
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "children": [
              {
                "branchesClass": "coverage-unknown",
                "branchesCoverage": "-",
                "coverage": "90.0",
                "coverageClass": "coverage-high",
                "functionsClass": "coverage-high",
                "functionsCoverage": "95.8",
                "isDirectory": false,
                "linesClass": "coverage-high",
                "linesCoverage": "90.0",
                "linesExec": "18",
                "linesTotal": "20",
                "link": "index.dynamic_invoke.hpp.8955ae16b874cb073f67cb792b727280.html",
                "name": "dynamic_invoke.hpp"
              },
              {
                "branchesClass": "coverage-unknown",
                "branchesCoverage": "-",
                "coverage": "100.0",
                "coverageClass": "coverage-high",
                "functionsClass": "coverage-high",
                "functionsCoverage": "100.0",
                "isDirectory": false,
                "linesClass": "coverage-high",
                "linesCoverage": "100.0",
                "linesExec": "7",
                "linesTotal": "7",
                "link": "index.router_base.hpp.ce452759a18b554ba292ad15d9d55685.html",
                "name": "router_base.hpp"
              }
            ],
            "coverage": "92.6",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "96.8",
            "isDirectory": true,
            "linesClass": "coverage-high",
            "linesCoverage": "92.6",
            "linesExec": "25",
            "linesTotal": "27",
            "link": "index.detail.4acd75487363d30d8eb80dca6fc79bae.html",
            "name": "detail"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "100.0",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "100.0",
            "linesExec": "9",
            "linesTotal": "9",
            "link": "index.http_worker.hpp.c4e3777a8c2d9c1f9cbe7fda48e0c25b.html",
            "name": "http_worker.hpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "92.9",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "91.7",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "92.9",
            "linesExec": "26",
            "linesTotal": "28",
            "link": "index.route_handler.hpp.2576fd1e6a5600195e108cf45680e01b.html",
            "name": "route_handler.hpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "99.1",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "97.8",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "99.1",
            "linesExec": "107",
            "linesTotal": "108",
            "link": "index.router.hpp.665834536bfb641b424cd21b5d2d90e0.html",
            "name": "router.hpp"
          }
        ],
        "coverage": "97.1",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-high",
        "functionsCoverage": "97.7",
        "isDirectory": true,
        "linesClass": "coverage-high",
        "linesCoverage": "97.1",
        "linesExec": "167",
        "linesTotal": "172",
        "link": "index.server.fb4fb92a0c9ef50980789e23bb00a00a.html",
        "name": "server"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "children": [
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "children": [
              {
                "branchesClass": "coverage-unknown",
                "branchesCoverage": "-",
                "coverage": "100.0",
                "coverageClass": "coverage-high",
                "functionsClass": "coverage-high",
                "functionsCoverage": "100.0",
                "isDirectory": false,
                "linesClass": "coverage-high",
                "linesCoverage": "100.0",
                "linesExec": "3",
                "linesTotal": "3",
                "link": "index.error.hpp.523888a0705cdd2f5db33f2cbf1742dc.html",
                "name": "error.hpp"
              }
            ],
            "coverage": "100.0",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": true,
            "linesClass": "coverage-high",
            "linesCoverage": "100.0",
            "linesExec": "3",
            "linesTotal": "3",
            "link": "index.impl.801ed5030631b74cb89026bdffe7b842.html",
            "name": "impl"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "100.0",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "100.0",
            "linesExec": "1",
            "linesTotal": "1",
            "link": "index.deflate.hpp.b1d6ca4a6ecdcb9d907fefc3918bd62c.html",
            "name": "deflate.hpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "100.0",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "100.0",
            "linesExec": "1",
            "linesTotal": "1",
            "link": "index.inflate.hpp.c586f71c42fa5598f50a18fc3a9eccab.html",
            "name": "inflate.hpp"
          }
        ],
        "coverage": "100.0",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-high",
        "functionsCoverage": "100.0",
        "isDirectory": true,
        "linesClass": "coverage-high",
        "linesCoverage": "100.0",
        "linesExec": "5",
        "linesTotal": "5",
        "link": "index.zlib.231b8f2cc1967c5c91035d78302e1031.html",
        "name": "zlib"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "children": [
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "children": [
              {
                "branchesClass": "coverage-unknown",
                "branchesCoverage": "-",
                "coverage": "100.0",
                "coverageClass": "coverage-high",
                "functionsClass": "coverage-high",
                "functionsCoverage": "100.0",
                "isDirectory": false,
                "linesClass": "coverage-high",
                "linesCoverage": "100.0",
                "linesExec": "3",
                "linesTotal": "3",
                "link": "index.error.hpp.39ea7fc328ca78bd83d43d0ef21eb228.html",
                "name": "error.hpp"
              }
            ],
            "coverage": "100.0",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": true,
            "linesClass": "coverage-high",
            "linesCoverage": "100.0",
            "linesExec": "3",
            "linesTotal": "3",
            "link": "index.impl.9f7a84b92aef5f588c8280cc4e90dec5.html",
            "name": "impl"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "100.0",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "100.0",
            "linesExec": "1",
            "linesTotal": "1",
            "link": "index.compress.hpp.b41b4328ba30f1089205c024dd6d22b1.html",
            "name": "compress.hpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "100.0",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "100.0",
            "linesExec": "1",
            "linesTotal": "1",
            "link": "index.decompress.hpp.6554d95c3d82112c5f855c0df371f775.html",
            "name": "decompress.hpp"
          }
        ],
        "coverage": "100.0",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-high",
        "functionsCoverage": "100.0",
        "isDirectory": true,
        "linesClass": "coverage-high",
        "linesCoverage": "100.0",
        "linesExec": "5",
        "linesTotal": "5",
        "link": "index.zstd.0f5f6a8080615b63d5e4f41947fe4259.html",
        "name": "zstd"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "coverage": "94.5",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-high",
        "functionsCoverage": "96.3",
        "isDirectory": false,
        "linesClass": "coverage-high",
        "linesCoverage": "94.5",
        "linesExec": "86",
        "linesTotal": "91",
        "link": "index.bcrypt.hpp.5d4460a254aa18c47609fe08195a93dc.html",
        "name": "bcrypt.hpp"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "coverage": "100.0",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-high",
        "functionsCoverage": "100.0",
        "isDirectory": false,
        "linesClass": "coverage-high",
        "linesCoverage": "100.0",
        "linesExec": "3",
        "linesTotal": "3",
        "link": "index.config.hpp.cb42aa1d3ca69c408759f5b60f38779c.html",
        "name": "config.hpp"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "coverage": "100.0",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-high",
        "functionsCoverage": "100.0",
        "isDirectory": false,
        "linesClass": "coverage-high",
        "linesCoverage": "100.0",
        "linesExec": "4",
        "linesTotal": "4",
        "link": "index.datastore.hpp.9285497c230061c41982847fd947f0ac.html",
        "name": "datastore.hpp"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "coverage": "100.0",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-high",
        "functionsCoverage": "100.0",
        "isDirectory": false,
        "linesClass": "coverage-high",
        "linesCoverage": "100.0",
        "linesExec": "28",
        "linesTotal": "28",
        "link": "index.fields.hpp.889f1bac55cee2ffe07dc3165658dd0e.html",
        "name": "fields.hpp"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "coverage": "100.0",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-high",
        "functionsCoverage": "100.0",
        "isDirectory": false,
        "linesClass": "coverage-high",
        "linesCoverage": "100.0",
        "linesExec": "44",
        "linesTotal": "44",
        "link": "index.fields_base.hpp.948a49624141a45d308e999ab4776040.html",
        "name": "fields_base.hpp"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "coverage": "91.0",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-high",
        "functionsCoverage": "100.0",
        "isDirectory": false,
        "linesClass": "coverage-high",
        "linesCoverage": "91.0",
        "linesExec": "61",
        "linesTotal": "67",
        "link": "index.file.hpp.3893e7b60cd7215cc608e2977a385b59.html",
        "name": "file.hpp"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "coverage": "100.0",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-high",
        "functionsCoverage": "100.0",
        "isDirectory": false,
        "linesClass": "coverage-high",
        "linesCoverage": "100.0",
        "linesExec": "11",
        "linesTotal": "11",
        "link": "index.message_base.hpp.0fe86be551b2a99eddd68557fc168da1.html",
        "name": "message_base.hpp"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "coverage": "100.0",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-high",
        "functionsCoverage": "100.0",
        "isDirectory": false,
        "linesClass": "coverage-high",
        "linesCoverage": "100.0",
        "linesExec": "1",
        "linesTotal": "1",
        "link": "index.metadata.hpp.8861b3c790a08e7759e27d0c0e89dd04.html",
        "name": "metadata.hpp"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "coverage": "100.0",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-medium",
        "functionsCoverage": "78.6",
        "isDirectory": false,
        "linesClass": "coverage-high",
        "linesCoverage": "100.0",
        "linesExec": "19",
        "linesTotal": "19",
        "link": "index.parser.hpp.2e9122d80150cdff7cbc6b60ff3bf525.html",
        "name": "parser.hpp"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "coverage": "100.0",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-high",
        "functionsCoverage": "100.0",
        "isDirectory": false,
        "linesClass": "coverage-high",
        "linesCoverage": "100.0",
        "linesExec": "40",
        "linesTotal": "40",
        "link": "index.request.hpp.13dd4cecfdef46e5a7878cfae6876819.html",
        "name": "request.hpp"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "coverage": "100.0",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-high",
        "functionsCoverage": "100.0",
        "isDirectory": false,
        "linesClass": "coverage-high",
        "linesCoverage": "100.0",
        "linesExec": "36",
        "linesTotal": "36",
        "link": "index.request_base.hpp.c1464336f8fb3cc48d0c746307d89032.html",
        "name": "request_base.hpp"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "coverage": "100.0",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-high",
        "functionsCoverage": "100.0",
        "isDirectory": false,
        "linesClass": "coverage-high",
        "linesCoverage": "100.0",
        "linesExec": "6",
        "linesTotal": "6",
        "link": "index.request_parser.hpp.93a0dd3ceea7be89b7a63c4aab658e50.html",
        "name": "request_parser.hpp"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "coverage": "100.0",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-high",
        "functionsCoverage": "100.0",
        "isDirectory": false,
        "linesClass": "coverage-high",
        "linesCoverage": "100.0",
        "linesExec": "40",
        "linesTotal": "40",
        "link": "index.response.hpp.2f447c3debc86895cf652afe84e59990.html",
        "name": "response.hpp"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "coverage": "96.6",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-high",
        "functionsCoverage": "100.0",
        "isDirectory": false,
        "linesClass": "coverage-high",
        "linesCoverage": "96.6",
        "linesExec": "28",
        "linesTotal": "29",
        "link": "index.response_base.hpp.8be452b52183da1302e84b11420d5767.html",
        "name": "response_base.hpp"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "coverage": "100.0",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-high",
        "functionsCoverage": "100.0",
        "isDirectory": false,
        "linesClass": "coverage-high",
        "linesCoverage": "100.0",
        "linesExec": "6",
        "linesTotal": "6",
        "link": "index.response_parser.hpp.699ab70efbb06f3575b0a9f62d59f540.html",
        "name": "response_parser.hpp"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "coverage": "100.0",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-high",
        "functionsCoverage": "100.0",
        "isDirectory": false,
        "linesClass": "coverage-high",
        "linesCoverage": "100.0",
        "linesExec": "27",
        "linesTotal": "27",
        "link": "index.serializer.hpp.efd7df8a0f2affc58c8c6f6c13a39d98.html",
        "name": "serializer.hpp"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "coverage": "100.0",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-high",
        "functionsCoverage": "100.0",
        "isDirectory": false,
        "linesClass": "coverage-high",
        "linesCoverage": "100.0",
        "linesExec": "16",
        "linesTotal": "16",
        "link": "index.static_request.hpp.fdb8019345d78e422c14e371736b47ae.html",
        "name": "static_request.hpp"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "coverage": "100.0",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-high",
        "functionsCoverage": "100.0",
        "isDirectory": false,
        "linesClass": "coverage-high",
        "linesCoverage": "100.0",
        "linesExec": "16",
        "linesTotal": "16",
        "link": "index.static_response.hpp.d1935de68e9440e5ec57713b789ff328.html",
        "name": "static_response.hpp"
      }
    ],
    "coverage": "96.5",
    "coverageClass": "coverage-high",
    "functionsClass": "coverage-high",
    "functionsCoverage": "90.7",
    "isDirectory": true,
    "linesClass": "coverage-high",
    "linesCoverage": "96.5",
    "linesExec": "1701",
    "linesTotal": "1765",
    "link": "index.http.a39f459d0ac76d49f63298b9c0f478fa.html",
    "name": "include/boost/http"
  },
  {
    "branchesClass": "coverage-unknown",
    "branchesCoverage": "-",
    "children": [
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "children": [
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "92.8",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "92.8",
            "linesExec": "64",
            "linesTotal": "69",
            "link": "index.base64.cpp.73cb92392d556a38c31579c89297cb65.html",
            "name": "base64.cpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "100.0",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "100.0",
            "linesExec": "101",
            "linesTotal": "101",
            "link": "index.blowfish.cpp.f21a45468e8d9a71980c4c5c0f4588e8.html",
            "name": "blowfish.cpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "86.5",
            "coverageClass": "coverage-medium",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-medium",
            "linesCoverage": "86.5",
            "linesExec": "64",
            "linesTotal": "74",
            "link": "index.crypt.cpp.12e62457e36b88666807979445bc9af1.html",
            "name": "crypt.cpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "44.4",
            "coverageClass": "coverage-low",
            "functionsClass": "coverage-low",
            "functionsCoverage": "50.0",
            "isDirectory": false,
            "linesClass": "coverage-low",
            "linesCoverage": "44.4",
            "linesExec": "4",
            "linesTotal": "9",
            "link": "index.error.cpp.35526e76be49f30a7778bc1c1fe2f544.html",
            "name": "error.cpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "77.4",
            "coverageClass": "coverage-medium",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-medium",
            "linesCoverage": "77.4",
            "linesExec": "48",
            "linesTotal": "62",
            "link": "index.hash.cpp.e67130be447ea491664b7a0dd41fc4fe.html",
            "name": "hash.cpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "61.5",
            "coverageClass": "coverage-low",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-low",
            "linesCoverage": "61.5",
            "linesExec": "8",
            "linesTotal": "13",
            "link": "index.random.cpp.d8fb8e29d71fb8f1cc565bd1af90e245.html",
            "name": "random.cpp"
          }
        ],
        "coverage": "88.1",
        "coverageClass": "coverage-medium",
        "functionsClass": "coverage-high",
        "functionsCoverage": "95.7",
        "isDirectory": true,
        "linesClass": "coverage-medium",
        "linesCoverage": "88.1",
        "linesExec": "289",
        "linesTotal": "328",
        "link": "index.bcrypt.0b5b72cd5f82d7ac8b4f917beb490f7f.html",
        "name": "bcrypt"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "children": [
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "0.0",
            "coverageClass": "coverage-none",
            "functionsClass": "coverage-none",
            "functionsCoverage": "0.0",
            "isDirectory": false,
            "linesClass": "coverage-none",
            "linesCoverage": "0.0",
            "linesExec": "0",
            "linesTotal": "36",
            "link": "index.error.cpp.f5412aa038e74830517ab8d8fb2383b3.html",
            "name": "error.cpp"
          }
        ],
        "coverage": "0.0",
        "coverageClass": "coverage-none",
        "functionsClass": "coverage-none",
        "functionsCoverage": "0.0",
        "isDirectory": true,
        "linesClass": "coverage-none",
        "linesCoverage": "0.0",
        "linesExec": "0",
        "linesTotal": "36",
        "link": "index.brotli.b530400a120229e38d8c8c268c58ec79.html",
        "name": "brotli"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "children": [
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "60.4",
            "coverageClass": "coverage-low",
            "functionsClass": "coverage-low",
            "functionsCoverage": "63.6",
            "isDirectory": false,
            "linesClass": "coverage-low",
            "linesCoverage": "60.4",
            "linesExec": "32",
            "linesTotal": "53",
            "link": "index.polystore.cpp.47aef0abe20353fd56e3cdf2e53fe6aa.html",
            "name": "polystore.cpp"
          }
        ],
        "coverage": "60.4",
        "coverageClass": "coverage-low",
        "functionsClass": "coverage-low",
        "functionsCoverage": "63.6",
        "isDirectory": true,
        "linesClass": "coverage-low",
        "linesCoverage": "60.4",
        "linesExec": "32",
        "linesTotal": "53",
        "link": "index.core.af69d6ecdd80a14b4e4df0784d3dc8d9.html",
        "name": "core"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "children": [
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "76.5",
            "coverageClass": "coverage-medium",
            "functionsClass": "coverage-medium",
            "functionsCoverage": "80.0",
            "isDirectory": false,
            "linesClass": "coverage-medium",
            "linesCoverage": "76.5",
            "linesExec": "26",
            "linesTotal": "34",
            "link": "index.array_of_const_buffers.cpp.7d1bab1c133b5e8d0e38c5f8da60582d.html",
            "name": "array_of_const_buffers.cpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "100.0",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "100.0",
            "linesExec": "10",
            "linesTotal": "10",
            "link": "index.array_of_const_buffers.hpp.5cdc6dbb6a63c6ea2d6bf411d0213955.html",
            "name": "array_of_const_buffers.hpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "100.0",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-unknown",
            "functionsCoverage": "-",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "100.0",
            "linesExec": "2",
            "linesTotal": "2",
            "link": "index.buffer_utils.hpp.6da2bef17d046cc5b82b5d2618f1d457.html",
            "name": "buffer_utils.hpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "97.0",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "97.0",
            "linesExec": "32",
            "linesTotal": "33",
            "link": "index.circular_buffer.cpp.2bee4dea6f300a884721dae71a9bf79c.html",
            "name": "circular_buffer.cpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "63.6",
            "coverageClass": "coverage-low",
            "functionsClass": "coverage-low",
            "functionsCoverage": "63.6",
            "isDirectory": false,
            "linesClass": "coverage-low",
            "linesCoverage": "63.6",
            "linesExec": "21",
            "linesTotal": "33",
            "link": "index.except.cpp.4645be379eebfa472ccd344a47a6674d.html",
            "name": "except.cpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "83.4",
            "coverageClass": "coverage-medium",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-medium",
            "linesCoverage": "83.4",
            "linesExec": "126",
            "linesTotal": "151",
            "link": "index.file_posix.cpp.748c7bd15f1bd86db5c9828890232679.html",
            "name": "file_posix.cpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "84.9",
            "coverageClass": "coverage-medium",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-medium",
            "linesCoverage": "84.9",
            "linesExec": "107",
            "linesTotal": "126",
            "link": "index.file_stdio.cpp.d4e8e5add69cdac62bf438118b25deaa.html",
            "name": "file_stdio.cpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "0.0",
            "coverageClass": "coverage-none",
            "functionsClass": "coverage-none",
            "functionsCoverage": "0.0",
            "isDirectory": false,
            "linesClass": "coverage-none",
            "linesCoverage": "0.0",
            "linesExec": "0",
            "linesTotal": "36",
            "link": "index.filter.cpp.94422c7dca504d223d69b3d947a52e85.html",
            "name": "filter.cpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "0.0",
            "coverageClass": "coverage-none",
            "functionsClass": "coverage-none",
            "functionsCoverage": "0.0",
            "isDirectory": false,
            "linesClass": "coverage-none",
            "linesCoverage": "0.0",
            "linesExec": "0",
            "linesTotal": "3",
            "link": "index.filter.hpp.b3cab868e787689174fefb5c0c29564c.html",
            "name": "filter.hpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "92.6",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "95.5",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "92.6",
            "linesExec": "588",
            "linesTotal": "635",
            "link": "index.header.cpp.9552052b73cdaf55ff4ca5603409b8d6.html",
            "name": "header.cpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "100.0",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "100.0",
            "linesExec": "25",
            "linesTotal": "25",
            "link": "index.move_chars.hpp.17447e31379167e317ad6cb5afee6a55.html",
            "name": "move_chars.hpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "100.0",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "100.0",
            "linesExec": "25",
            "linesTotal": "25",
            "link": "index.number_string.hpp.1a98bd15e10b01d1140c41898f4d8261.html",
            "name": "number_string.hpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "55.1",
            "coverageClass": "coverage-low",
            "functionsClass": "coverage-low",
            "functionsCoverage": "63.6",
            "isDirectory": false,
            "linesClass": "coverage-low",
            "linesCoverage": "55.1",
            "linesExec": "49",
            "linesTotal": "89",
            "link": "index.workspace.cpp.66c2b627f841c16fb82abb20c7aaa954.html",
            "name": "workspace.cpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "0.0",
            "coverageClass": "coverage-none",
            "functionsClass": "coverage-none",
            "functionsCoverage": "0.0",
            "isDirectory": false,
            "linesClass": "coverage-none",
            "linesCoverage": "0.0",
            "linesExec": "0",
            "linesTotal": "10",
            "link": "index.zlib_filter_base.hpp.16c837561db2adda2b99dfebd690f583.html",
            "name": "zlib_filter_base.hpp"
          }
        ],
        "coverage": "83.4",
        "coverageClass": "coverage-medium",
        "functionsClass": "coverage-medium",
        "functionsCoverage": "86.1",
        "isDirectory": true,
        "linesClass": "coverage-medium",
        "linesCoverage": "83.4",
        "linesExec": "1011",
        "linesTotal": "1212",
        "link": "index.detail.8efc291289a587be34f5679ec1c70004.html",
        "name": "detail"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "children": [
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "0.0",
            "coverageClass": "coverage-none",
            "functionsClass": "coverage-none",
            "functionsCoverage": "0.0",
            "isDirectory": false,
            "linesClass": "coverage-none",
            "linesCoverage": "0.0",
            "linesExec": "0",
            "linesTotal": "6",
            "link": "index.json_body.cpp.61d5e7d8518d55af7b08e17de4887c1d.html",
            "name": "json_body.cpp"
          }
        ],
        "coverage": "0.0",
        "coverageClass": "coverage-none",
        "functionsClass": "coverage-none",
        "functionsCoverage": "0.0",
        "isDirectory": true,
        "linesClass": "coverage-none",
        "linesCoverage": "0.0",
        "linesExec": "0",
        "linesTotal": "6",
        "link": "index.json.962152396eeef59d52489bbb9f714d38.html",
        "name": "json"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "children": [
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "children": [
              {
                "branchesClass": "coverage-unknown",
                "branchesCoverage": "-",
                "coverage": "95.8",
                "coverageClass": "coverage-high",
                "functionsClass": "coverage-high",
                "functionsCoverage": "100.0",
                "isDirectory": false,
                "linesClass": "coverage-high",
                "linesCoverage": "95.8",
                "linesExec": "159",
                "linesTotal": "166",
                "link": "index.rules.cpp.6c6ed11c5176e0e02da2b28daeef78bf.html",
                "name": "rules.cpp"
              },
              {
                "branchesClass": "coverage-unknown",
                "branchesCoverage": "-",
                "coverage": "100.0",
                "coverageClass": "coverage-high",
                "functionsClass": "coverage-high",
                "functionsCoverage": "100.0",
                "isDirectory": false,
                "linesClass": "coverage-high",
                "linesCoverage": "100.0",
                "linesExec": "3",
                "linesTotal": "3",
                "link": "index.rules.hpp.4826b198882c870a451fe7d51e3a7b7f.html",
                "name": "rules.hpp"
              },
              {
                "branchesClass": "coverage-unknown",
                "branchesCoverage": "-",
                "coverage": "88.3",
                "coverageClass": "coverage-medium",
                "functionsClass": "coverage-high",
                "functionsCoverage": "100.0",
                "isDirectory": false,
                "linesClass": "coverage-medium",
                "linesCoverage": "88.3",
                "linesExec": "53",
                "linesTotal": "60",
                "link": "index.transfer_coding_rule.cpp.ae3c0e328ea6543e395a008db2e1747e.html",
                "name": "transfer_coding_rule.cpp"
              }
            ],
            "coverage": "93.9",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": true,
            "linesClass": "coverage-high",
            "linesCoverage": "93.9",
            "linesExec": "215",
            "linesTotal": "229",
            "link": "index.detail.0ddf9a438c378b52671e24841a287d77.html",
            "name": "detail"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "92.6",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "92.6",
            "linesExec": "25",
            "linesTotal": "27",
            "link": "index.combine_field_values.cpp.600a96a65d582b23ca83ab2281716893.html",
            "name": "combine_field_values.cpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "0.0",
            "coverageClass": "coverage-none",
            "functionsClass": "coverage-none",
            "functionsCoverage": "0.0",
            "isDirectory": false,
            "linesClass": "coverage-none",
            "linesCoverage": "0.0",
            "linesExec": "0",
            "linesTotal": "2",
            "link": "index.parameter.cpp.369d20ea8850ef13e0bf8e4992cd3ccc.html",
            "name": "parameter.cpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "82.8",
            "coverageClass": "coverage-medium",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-medium",
            "linesCoverage": "82.8",
            "linesExec": "24",
            "linesTotal": "29",
            "link": "index.quoted_token_rule.cpp.6a4997b80054c0169f5f0d9045b73a16.html",
            "name": "quoted_token_rule.cpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "93.3",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "93.3",
            "linesExec": "14",
            "linesTotal": "15",
            "link": "index.upgrade_rule.cpp.c77787cb80c5f1d256394e83d95b5e68.html",
            "name": "upgrade_rule.cpp"
          }
        ],
        "coverage": "92.1",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-high",
        "functionsCoverage": "94.1",
        "isDirectory": true,
        "linesClass": "coverage-high",
        "linesCoverage": "92.1",
        "linesExec": "278",
        "linesTotal": "302",
        "link": "index.rfc.8c12d4cb96266f534a73528ba6cea752.html",
        "name": "rfc"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "children": [
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "children": [
              {
                "branchesClass": "coverage-unknown",
                "branchesCoverage": "-",
                "coverage": "94.1",
                "coverageClass": "coverage-high",
                "functionsClass": "coverage-high",
                "functionsCoverage": "100.0",
                "isDirectory": false,
                "linesClass": "coverage-high",
                "linesCoverage": "94.1",
                "linesExec": "32",
                "linesTotal": "34",
                "link": "index.any_router.hpp.c851d411cd37a6afccf35ffb6652a905.html",
                "name": "any_router.hpp"
              },
              {
                "branchesClass": "coverage-unknown",
                "branchesCoverage": "-",
                "coverage": "63.3",
                "coverageClass": "coverage-low",
                "functionsClass": "coverage-low",
                "functionsCoverage": "66.7",
                "isDirectory": false,
                "linesClass": "coverage-low",
                "linesCoverage": "63.3",
                "linesExec": "38",
                "linesTotal": "60",
                "link": "index.pct_decode.cpp.b6fa66fd16ffec1d7cc8c3a6a446e341.html",
                "name": "pct_decode.cpp"
              },
              {
                "branchesClass": "coverage-unknown",
                "branchesCoverage": "-",
                "coverage": "94.6",
                "coverageClass": "coverage-high",
                "functionsClass": "coverage-high",
                "functionsCoverage": "100.0",
                "isDirectory": false,
                "linesClass": "coverage-high",
                "linesCoverage": "94.6",
                "linesExec": "35",
                "linesTotal": "37",
                "link": "index.route_match.cpp.4bdc7ca7060dc3e94e37da88000d9a43.html",
                "name": "route_match.cpp"
              },
              {
                "branchesClass": "coverage-unknown",
                "branchesCoverage": "-",
                "coverage": "100.0",
                "coverageClass": "coverage-high",
                "functionsClass": "coverage-high",
                "functionsCoverage": "100.0",
                "isDirectory": false,
                "linesClass": "coverage-high",
                "linesCoverage": "100.0",
                "linesExec": "1",
                "linesTotal": "1",
                "link": "index.route_match.hpp.6e53009ec89266d1c557532a53a86864.html",
                "name": "route_match.hpp"
              },
              {
                "branchesClass": "coverage-unknown",
                "branchesCoverage": "-",
                "coverage": "100.0",
                "coverageClass": "coverage-high",
                "functionsClass": "coverage-high",
                "functionsCoverage": "100.0",
                "isDirectory": false,
                "linesClass": "coverage-high",
                "linesCoverage": "100.0",
                "linesExec": "25",
                "linesTotal": "25",
                "link": "index.stable_string.hpp.cf94d83a5d40d90675c426aec2f624fb.html",
                "name": "stable_string.hpp"
              }
            ],
            "coverage": "83.4",
            "coverageClass": "coverage-medium",
            "functionsClass": "coverage-high",
            "functionsCoverage": "95.2",
            "isDirectory": true,
            "linesClass": "coverage-medium",
            "linesCoverage": "83.4",
            "linesExec": "131",
            "linesTotal": "157",
            "link": "index.detail.c47ad0ca485c01099f0da4eddae18063.html",
            "name": "detail"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "86.1",
            "coverageClass": "coverage-medium",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-medium",
            "linesCoverage": "86.1",
            "linesExec": "272",
            "linesTotal": "316",
            "link": "index.accepts.cpp.136c7c58d33b13649dd884222e0cedd3.html",
            "name": "accepts.cpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "91.6",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "94.1",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "91.6",
            "linesExec": "218",
            "linesTotal": "238",
            "link": "index.any_router.cpp.d8170eca22121b2618d4929a965ce5a5.html",
            "name": "any_router.cpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "54.2",
            "coverageClass": "coverage-low",
            "functionsClass": "coverage-high",
            "functionsCoverage": "90.9",
            "isDirectory": false,
            "linesClass": "coverage-low",
            "linesCoverage": "54.2",
            "linesExec": "32",
            "linesTotal": "59",
            "link": "index.cors.cpp.bbb1c466c89a51720cbeb8d3a3549c79.html",
            "name": "cors.cpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "0.0",
            "coverageClass": "coverage-none",
            "functionsClass": "coverage-none",
            "functionsCoverage": "0.0",
            "isDirectory": false,
            "linesClass": "coverage-none",
            "linesCoverage": "0.0",
            "linesExec": "0",
            "linesTotal": "22",
            "link": "index.encode_url.cpp.e639686084f331a4928f88d6d8c029be.html",
            "name": "encode_url.cpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "0.0",
            "coverageClass": "coverage-none",
            "functionsClass": "coverage-none",
            "functionsCoverage": "0.0",
            "isDirectory": false,
            "linesClass": "coverage-none",
            "linesCoverage": "0.0",
            "linesExec": "0",
            "linesTotal": "25",
            "link": "index.escape_html.cpp.6ebe83ea2298cf36d97cdc4425ab8bb0.html",
            "name": "escape_html.cpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "77.8",
            "coverageClass": "coverage-medium",
            "functionsClass": "coverage-medium",
            "functionsCoverage": "75.0",
            "isDirectory": false,
            "linesClass": "coverage-medium",
            "linesCoverage": "77.8",
            "linesExec": "21",
            "linesTotal": "27",
            "link": "index.etag.cpp.403f9fd39b2753cc313043eddf4cf687.html",
            "name": "etag.cpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "13.5",
            "coverageClass": "coverage-low",
            "functionsClass": "coverage-low",
            "functionsCoverage": "25.0",
            "isDirectory": false,
            "linesClass": "coverage-low",
            "linesCoverage": "13.5",
            "linesExec": "5",
            "linesTotal": "37",
            "link": "index.fresh.cpp.7d7d72fcd01d7744666ce32249828213.html",
            "name": "fresh.cpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "100.0",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "100.0",
            "linesExec": "10",
            "linesTotal": "10",
            "link": "index.http_worker.cpp.8157a36f705d2f5bf4e71291c44044f1.html",
            "name": "http_worker.cpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "0.0",
            "coverageClass": "coverage-none",
            "functionsClass": "coverage-none",
            "functionsCoverage": "0.0",
            "isDirectory": false,
            "linesClass": "coverage-none",
            "linesCoverage": "0.0",
            "linesExec": "0",
            "linesTotal": "26",
            "link": "index.mime_db.cpp.ac373363f8f74b274ed4b9e1e67e51d1.html",
            "name": "mime_db.cpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "50.7",
            "coverageClass": "coverage-low",
            "functionsClass": "coverage-low",
            "functionsCoverage": "57.1",
            "isDirectory": false,
            "linesClass": "coverage-low",
            "linesCoverage": "50.7",
            "linesExec": "34",
            "linesTotal": "67",
            "link": "index.mime_types.cpp.4964c31d05e5090fefb0dfc9245484e8.html",
            "name": "mime_types.cpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "0.0",
            "coverageClass": "coverage-none",
            "functionsClass": "coverage-none",
            "functionsCoverage": "0.0",
            "isDirectory": false,
            "linesClass": "coverage-none",
            "linesCoverage": "0.0",
            "linesExec": "0",
            "linesTotal": "91",
            "link": "index.range_parser.cpp.1fe00115375c1b8a8f541a2b90ca6529.html",
            "name": "range_parser.cpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "93.6",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "93.6",
            "linesExec": "248",
            "linesTotal": "265",
            "link": "index.route_abnf.cpp.f8797ec3570677644d2d78f5f27633f7.html",
            "name": "route_abnf.cpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "100.0",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "100.0",
            "linesExec": "5",
            "linesTotal": "5",
            "link": "index.route_abnf.hpp.75cdf273fb1d67b90664b2754b66eba0.html",
            "name": "route_abnf.hpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "40.0",
            "coverageClass": "coverage-low",
            "functionsClass": "coverage-low",
            "functionsCoverage": "50.0",
            "isDirectory": false,
            "linesClass": "coverage-low",
            "linesCoverage": "40.0",
            "linesExec": "2",
            "linesTotal": "5",
            "link": "index.router.cpp.6563dd2c4d0fa92343803c54e2f78df3.html",
            "name": "router.cpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "59.1",
            "coverageClass": "coverage-low",
            "functionsClass": "coverage-low",
            "functionsCoverage": "66.7",
            "isDirectory": false,
            "linesClass": "coverage-low",
            "linesCoverage": "59.1",
            "linesExec": "26",
            "linesTotal": "44",
            "link": "index.router_types.cpp.c78d38a99e040dc5fbdaa6ddbe6b34eb.html",
            "name": "router_types.cpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "0.0",
            "coverageClass": "coverage-none",
            "functionsClass": "coverage-none",
            "functionsCoverage": "0.0",
            "isDirectory": false,
            "linesClass": "coverage-none",
            "linesCoverage": "0.0",
            "linesExec": "0",
            "linesTotal": "85",
            "link": "index.send_file.cpp.bfd71a35b1e78b0bc00333cd99de5505.html",
            "name": "send_file.cpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "0.0",
            "coverageClass": "coverage-none",
            "functionsClass": "coverage-none",
            "functionsCoverage": "0.0",
            "isDirectory": false,
            "linesClass": "coverage-none",
            "linesCoverage": "0.0",
            "linesExec": "0",
            "linesTotal": "137",
            "link": "index.serve_index.cpp.b66369a55c2c3596b981b3f459c6d8b7.html",
            "name": "serve_index.cpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "0.0",
            "coverageClass": "coverage-none",
            "functionsClass": "coverage-none",
            "functionsCoverage": "0.0",
            "isDirectory": false,
            "linesClass": "coverage-none",
            "linesCoverage": "0.0",
            "linesExec": "0",
            "linesTotal": "40",
            "link": "index.serve_static.cpp.33095e4f12fe4461fbe99fab6b31fc08.html",
            "name": "serve_static.cpp"
          },
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "100.0",
            "coverageClass": "coverage-high",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-high",
            "linesCoverage": "100.0",
            "linesExec": "18",
            "linesTotal": "18",
            "link": "index.statuses.cpp.4f280dd316e273395dfa849fb853355c.html",
            "name": "statuses.cpp"
          }
        ],
        "coverage": "61.1",
        "coverageClass": "coverage-low",
        "functionsClass": "coverage-low",
        "functionsCoverage": "72.6",
        "isDirectory": true,
        "linesClass": "coverage-low",
        "linesCoverage": "61.1",
        "linesExec": "1022",
        "linesTotal": "1674",
        "link": "index.server.be5b1592dd6f3dfe11fd6f648158ffee.html",
        "name": "server"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "children": [
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "0.0",
            "coverageClass": "coverage-none",
            "functionsClass": "coverage-none",
            "functionsCoverage": "0.0",
            "isDirectory": false,
            "linesClass": "coverage-none",
            "linesCoverage": "0.0",
            "linesExec": "0",
            "linesTotal": "15",
            "link": "index.error.cpp.6077fda8b2098e419720e34d6b4eda37.html",
            "name": "error.cpp"
          }
        ],
        "coverage": "0.0",
        "coverageClass": "coverage-none",
        "functionsClass": "coverage-none",
        "functionsCoverage": "0.0",
        "isDirectory": true,
        "linesClass": "coverage-none",
        "linesCoverage": "0.0",
        "linesExec": "0",
        "linesTotal": "15",
        "link": "index.zlib.01562381771ba05c467b8d7ffd9d89e2.html",
        "name": "zlib"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "children": [
          {
            "branchesClass": "coverage-unknown",
            "branchesCoverage": "-",
            "coverage": "22.9",
            "coverageClass": "coverage-low",
            "functionsClass": "coverage-high",
            "functionsCoverage": "100.0",
            "isDirectory": false,
            "linesClass": "coverage-low",
            "linesCoverage": "22.9",
            "linesExec": "8",
            "linesTotal": "35",
            "link": "index.error.cpp.61ab56f613c24417d5f305cdb650bf28.html",
            "name": "error.cpp"
          }
        ],
        "coverage": "22.9",
        "coverageClass": "coverage-low",
        "functionsClass": "coverage-high",
        "functionsCoverage": "100.0",
        "isDirectory": true,
        "linesClass": "coverage-low",
        "linesCoverage": "22.9",
        "linesExec": "8",
        "linesTotal": "35",
        "link": "index.zstd.7fa5566ab39110b98c000101f1baa89c.html",
        "name": "zstd"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "coverage": "18.6",
        "coverageClass": "coverage-low",
        "functionsClass": "coverage-low",
        "functionsCoverage": "28.6",
        "isDirectory": false,
        "linesClass": "coverage-low",
        "linesCoverage": "18.6",
        "linesExec": "11",
        "linesTotal": "59",
        "link": "index.application.cpp.10fce9dfb3ed544af04ec669e47a620a.html",
        "name": "application.cpp"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "coverage": "95.7",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-high",
        "functionsCoverage": "100.0",
        "isDirectory": false,
        "linesClass": "coverage-high",
        "linesCoverage": "95.7",
        "linesExec": "22",
        "linesTotal": "23",
        "link": "index.config.cpp.498d06653ad4a59db94a430cfa6f3937.html",
        "name": "config.cpp"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "coverage": "87.5",
        "coverageClass": "coverage-medium",
        "functionsClass": "coverage-high",
        "functionsCoverage": "100.0",
        "isDirectory": false,
        "linesClass": "coverage-medium",
        "linesCoverage": "87.5",
        "linesExec": "49",
        "linesTotal": "56",
        "link": "index.error.cpp.d3b44edfe5b0e6a13719399208fd02ce.html",
        "name": "error.cpp"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "coverage": "92.8",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-high",
        "functionsCoverage": "90.9",
        "isDirectory": false,
        "linesClass": "coverage-high",
        "linesCoverage": "92.8",
        "linesExec": "64",
        "linesTotal": "69",
        "link": "index.field.cpp.4ad10df75ab302c4b9ae78f758a32003.html",
        "name": "field.cpp"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "coverage": "97.4",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-high",
        "functionsCoverage": "98.6",
        "isDirectory": false,
        "linesClass": "coverage-high",
        "linesCoverage": "97.4",
        "linesExec": "685",
        "linesTotal": "703",
        "link": "index.fields_base.cpp.a5975054ea438b828f9367cbbc88fc7e.html",
        "name": "fields_base.cpp"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "coverage": "100.0",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-high",
        "functionsCoverage": "100.0",
        "isDirectory": false,
        "linesClass": "coverage-high",
        "linesCoverage": "100.0",
        "linesExec": "5",
        "linesTotal": "5",
        "link": "index.header_limits.cpp.89122504e14695cfb61696f448ee7814.html",
        "name": "header_limits.cpp"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "coverage": "94.7",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-high",
        "functionsCoverage": "100.0",
        "isDirectory": false,
        "linesClass": "coverage-high",
        "linesCoverage": "94.7",
        "linesExec": "71",
        "linesTotal": "75",
        "link": "index.message_base.cpp.2018d14f36f466bbb83da5ca97c1a647.html",
        "name": "message_base.cpp"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "coverage": "96.7",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-low",
        "functionsCoverage": "66.7",
        "isDirectory": false,
        "linesClass": "coverage-high",
        "linesCoverage": "96.7",
        "linesExec": "178",
        "linesTotal": "184",
        "link": "index.method.cpp.02ea8a41ceec8acdbdbcd3e5e7dfa3d0.html",
        "name": "method.cpp"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "coverage": "77.9",
        "coverageClass": "coverage-medium",
        "functionsClass": "coverage-medium",
        "functionsCoverage": "85.5",
        "isDirectory": false,
        "linesClass": "coverage-medium",
        "linesCoverage": "77.9",
        "linesExec": "535",
        "linesTotal": "687",
        "link": "index.parser.cpp.bd8f167953cc43fb7cba7d420fbd1d48.html",
        "name": "parser.cpp"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "coverage": "100.0",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-high",
        "functionsCoverage": "100.0",
        "isDirectory": false,
        "linesClass": "coverage-high",
        "linesCoverage": "100.0",
        "linesExec": "56",
        "linesTotal": "56",
        "link": "index.request_base.cpp.f8c7f6c3bd9210ec1e48c064daa42932.html",
        "name": "request_base.cpp"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "coverage": "100.0",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-high",
        "functionsCoverage": "100.0",
        "isDirectory": false,
        "linesClass": "coverage-high",
        "linesCoverage": "100.0",
        "linesExec": "6",
        "linesTotal": "6",
        "link": "index.request_parser.cpp.357574cfff6c0b8a612db30d8a06ebc1.html",
        "name": "request_parser.cpp"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "coverage": "95.1",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-high",
        "functionsCoverage": "100.0",
        "isDirectory": false,
        "linesClass": "coverage-high",
        "linesCoverage": "95.1",
        "linesExec": "39",
        "linesTotal": "41",
        "link": "index.response_base.cpp.55c4ed3a8e8c7901b51818afe4d50e89.html",
        "name": "response_base.cpp"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "coverage": "100.0",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-high",
        "functionsCoverage": "100.0",
        "isDirectory": false,
        "linesClass": "coverage-high",
        "linesCoverage": "100.0",
        "linesExec": "6",
        "linesTotal": "6",
        "link": "index.response_parser.cpp.4a4732258a56ad0beeb38bb5bde32520.html",
        "name": "response_parser.cpp"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "coverage": "62.8",
        "coverageClass": "coverage-low",
        "functionsClass": "coverage-medium",
        "functionsCoverage": "82.4",
        "isDirectory": false,
        "linesClass": "coverage-low",
        "linesCoverage": "62.8",
        "linesExec": "247",
        "linesTotal": "393",
        "link": "index.serializer.cpp.307cd5418da1f095a7a11da1a7a19343.html",
        "name": "serializer.cpp"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "coverage": "94.3",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-medium",
        "functionsCoverage": "80.0",
        "isDirectory": false,
        "linesClass": "coverage-high",
        "linesCoverage": "94.3",
        "linesExec": "82",
        "linesTotal": "87",
        "link": "index.status.cpp.28027b5db72cbf387fb5ba8d9f80b19f.html",
        "name": "status.cpp"
      },
      {
        "branchesClass": "coverage-unknown",
        "branchesCoverage": "-",
        "coverage": "100.0",
        "coverageClass": "coverage-high",
        "functionsClass": "coverage-high",
        "functionsCoverage": "100.0",
        "isDirectory": false,
        "linesClass": "coverage-high",
        "linesCoverage": "100.0",
        "linesExec": "9",
        "linesTotal": "9",
        "link": "index.version.cpp.fe1e38d934712fa3e000901095dbfeb6.html",
        "name": "version.cpp"
      }
    ],
    "coverage": "76.9",
    "coverageClass": "coverage-medium",
    "functionsClass": "coverage-medium",
    "functionsCoverage": "82.3",
    "isDirectory": true,
    "linesClass": "coverage-medium",
    "linesCoverage": "76.9",
    "linesExec": "4705",
    "linesTotal": "6120",
    "link": "index.src.25d902c24283ab8cfbac54dfa101ad31.html",
    "name": "src"
  }
];
