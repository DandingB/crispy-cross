#include "WindowContainer.h"
#include "../platform/Platform.h"

bool cxView::PointInView(int x, int y)
{
	int left, top, right, bottom;
	GetWindowRect(left, top, right, bottom);

	if (x > left and x < right and y > top and y < bottom)
		return true;
	else
		return false;
}

void cxView::GetWindowPos(int& x, int& y)
{
	if (!m_Parent)
	{
		x = m_Left;
		y = m_Top;
	}
	else
	{
		int x1, y1;
		m_Parent->GetWindowPos(x1, y1);
		x = x1 + m_Left;
		y = y1 + m_Top;
	}
}

void cxView::GetWindowRect(int& left, int& top, int& right, int& bottom)
{
	if (!m_Parent)
	{
		left = m_Left;
		top = m_Top;
		right = m_Right;
		bottom = m_Bottom;
	}
	else
	{
		int left1, top1, right1, bottom1;
		m_Parent->GetWindowRect(left1, top1, right1, bottom1);

		left = left1 + m_Left;
		top = top1 + m_Top;
		right = left1 + m_Right;
		bottom = top1 + m_Bottom;

		if (right > right1)
			right = right1;
			
		if (bottom > bottom1)
			bottom = bottom1;
	}
}

void cxView::AddView(cxView* view)
{
	m_SubViews.push_back(view);
	view->m_Parent = this;
}

cxWindowContainer::cxWindowContainer()
{
	glEnable(GL_MULTISAMPLE);

    // Create multisample framebuffer
    glGenFramebuffers(1, &multisampleFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, multisampleFBO);

    // Create multisample color buffer
    glGenRenderbuffers(1, &multisampleColorBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, multisampleColorBuffer);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 8, GL_RGBA8, 500, 500);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, multisampleColorBuffer);

    // Check framebuffer completeness
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
		cxLog(L"Framebuffer is incomplete: %x", status);
    }
}

cxWindowContainer::~cxWindowContainer()
{
	
}

void cxWindowContainer::AddView(cxView* view)
{
	m_SubViews.push_back(view);
	view->m_Parent = nullptr;
}

cxView* cxWindowContainer::GetChildView(int i)
{
	if (i >= 0 and i < m_SubViews.size())
		return m_SubViews[i];
	else
		return nullptr;
}


void cxWindowContainer::StartPaint()
{
	int width, height;
	GetClientSize(width, height);

	glBindFramebuffer(GL_FRAMEBUFFER, multisampleFBO);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glOrtho(0, width, height, 0, -1, 1);
	glViewport(0, 0, width, height);

	glClear(GL_COLOR_BUFFER_BIT);
}

void cxWindowContainer::EndPaint()
{
	int width, height;
	GetClientSize(width, height);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, multisampleFBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glFlush();
}

void cxWindowContainer::OnPaint()
{
	int width, height;
	GetClientSize(width, height);

	StartPaint();
	PaintSubviews(m_SubViews, 0, 0, width, height);
	EndPaint();
}


void cxWindowContainer::PaintSubviews(std::vector<cxView*>& views, int left, int top, int right, int bottom)
{
	int clientWidth, clientHeight;
	GetClientSize(clientWidth, clientHeight);

	for (cxView* view : views)
	{
		int x, y, width, height;

		x = left + view->m_Left;
		y = top + view->m_Top;
		width = view->m_Right - view->m_Left;
		height = view->m_Bottom - view->m_Top;

		if (x + width > right)
			width = right - x;
			
		if (y + height > bottom)
			height = bottom - y;

		if (width < 0 or height < 0)
			continue;


		glLoadIdentity();
		glMatrixMode(GL_PROJECTION);
		glOrtho(0, width, height, 0, -1, 1);
		glViewport(x, clientHeight - height - y, width, height);

		//cxLog(L"x: %d, y: %d, width: %d, height: %d", x, y, width, height);
		
		view->OnPaint();

		PaintSubviews(view->m_SubViews, x, y, x + width, y + height);
	}
}

void cxWindowContainer::OnSize(int width, int height)
{
	glBindRenderbuffer(GL_RENDERBUFFER, multisampleColorBuffer);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 8, GL_RGBA8, width, height);

	//HMODULE module = LoadLibraryA("opengl32.dll");
	//p = (void*)GetProcAddress(module, name);
}


void cxWindowContainer::OnMouseDown(cxMouseEvent event)
{
	for (int i = m_SubViews.size() - 1; i >= 0; i--)
	{
		cxView* view = m_SubViews[i];
		if ((event.x > view->m_Left) and (event.x < view->m_Right) and (event.y > view->m_Top) and (event.y < view->m_Bottom))
		{
			view->OnMouseDown({ event.x - view->m_Left, event.y - view->m_Top, event.button});
			break;
		}
	}
}

void cxWindowContainer::OnMouseUp(cxMouseEvent event)
{
	for (int i = m_SubViews.size() - 1; i >= 0; i--)
	{
		cxView* view = m_SubViews[i];
		if ((event.x > view->m_Left) and (event.x < view->m_Right) and (event.y > view->m_Top) and (event.y < view->m_Bottom))
		{
			view->OnMouseUp({ event.x - view->m_Left, event.y - view->m_Top, event.button });
			break;
		}
	}
}

void cxWindowContainer::OnMouseMove(cxMouseEvent event)
{
	for (int i = m_SubViews.size() - 1; i >= 0; i--)
	{
		cxView* view = m_SubViews[i];
		if ((event.x > view->m_Left) and (event.x < view->m_Right) and (event.y > view->m_Top) and (event.y < view->m_Bottom))
		{
			view->OnMouseMove({ event.x - view->m_Left, event.y - view->m_Top, event.button });
			break;
		}
	}
}
