#include "souistd.h"
#include "control/SHeaderCtrl.h"
#include "helper/DragWnd.h"

namespace SOUI
{
#define CX_HDITEM_MARGIN    4

    SHeaderCtrl::SHeaderCtrl(void)
        :m_bFixWidth(FALSE)
        ,m_bItemSwapEnable(TRUE)
        ,m_bSortHeader(TRUE)
        ,m_pSkinItem(GETBUILTINSKIN(SKIN_SYS_HEADER))
        ,m_pSkinSort(NULL)
        ,m_dwHitTest(-1)
        ,m_bDragging(FALSE)
        ,m_hDragImg(NULL)
    {
        m_bClipClient=TRUE;
        m_evtSet.addEvent(EventHeaderClick::EventID);
        m_evtSet.addEvent(EventHeaderItemChanged::EventID);
        m_evtSet.addEvent(EventHeaderItemChanging::EventID);
        m_evtSet.addEvent(EventHeaderItemSwap::EventID);
    }

    SHeaderCtrl::~SHeaderCtrl(void)
    {
    }

    int SHeaderCtrl::InsertItem( int iItem,LPCTSTR pszText,int nWidth, SHDSORTFLAG stFlag,LPARAM lParam )
    {
        SASSERT(pszText);
        SASSERT(nWidth>=0);
        if(iItem==-1) iItem=m_arrItems.GetCount();
        SHDITEM item;
        item.mask=0xFFFFFFFF;
        item.cx=nWidth;
        item.pszText=_tcsdup(pszText);
        item.cchTextMax=_tcslen(pszText);
        item.stFlag=stFlag;
        item.state=0;
        item.iOrder=iItem;
        item.lParam=lParam;
        m_arrItems.InsertAt(iItem,item);
        //需要更新列的序号
        for(size_t i=0;i<GetItemCount();i++)
        {
            if(i==iItem) continue;
            if(m_arrItems[i].iOrder>=iItem)
                m_arrItems[i].iOrder++;
        }
        Invalidate();
        return iItem;
    }

    BOOL SHeaderCtrl::GetItem( int iItem,SHDITEM *pItem )
    {
        if((UINT)iItem>=m_arrItems.GetCount()) return FALSE;
        if(pItem->mask & SHDI_TEXT)
        {
             if(pItem->cchTextMax<m_arrItems[iItem].cchTextMax) return FALSE;
             _tcscpy(pItem->pszText,m_arrItems[iItem].pszText);
        }
        if(pItem->mask & SHDI_WIDTH) pItem->cx=m_arrItems[iItem].cx;
        if(pItem->mask & SHDI_LPARAM) pItem->lParam=m_arrItems[iItem].lParam;
        if(pItem->mask & SHDI_SORTFLAG) pItem->stFlag=m_arrItems[iItem].stFlag;
        if(pItem->mask & SHDI_ORDER) pItem->iOrder=m_arrItems[iItem].iOrder;
        return TRUE;
    }

    void SHeaderCtrl::OnPaint(IRenderTarget * pRT )
    {
        SPainter painter;
        BeforePaint(pRT,painter);
        CRect rcClient;
        GetClientRect(&rcClient);
        CRect rcItem(rcClient.left,rcClient.top,rcClient.left,rcClient.bottom);
        for(UINT i=0;i<m_arrItems.GetCount();i++)
        {
            rcItem.left=rcItem.right;
            rcItem.right=rcItem.left+m_arrItems[i].cx;
            DrawItem(pRT,rcItem,m_arrItems.GetData()+i);
            if(rcItem.right>=rcClient.right) break;
        }
        if(rcItem.right<rcClient.right)
        {
            rcItem.left=rcItem.right;
            rcItem.right=rcClient.right;
            m_pSkinItem->Draw(pRT,rcItem,0);
        }
        AfterPaint(pRT,painter);
    }

    void SHeaderCtrl::DrawItem(IRenderTarget * pRT,CRect rcItem,const LPSHDITEM pItem )
    {
        if(m_pSkinItem) m_pSkinItem->Draw(pRT,rcItem,pItem->state);
        pRT->DrawText(pItem->pszText,pItem->cchTextMax,rcItem,m_style.GetTextAlign());
        if(pItem->stFlag==ST_NULL || !m_pSkinSort) return;
        CSize szSort=m_pSkinSort->GetSkinSize();
        CPoint ptSort;
        ptSort.y=rcItem.top+(rcItem.Height()-szSort.cy)/2;

        if(m_style.GetTextAlign()&DT_RIGHT)
            ptSort.x=rcItem.left+2;
        else
            ptSort.x=rcItem.right-szSort.cx-2;

        m_pSkinSort->Draw(pRT,CRect(ptSort,szSort),pItem->stFlag==ST_UP?0:1);
    }

    BOOL SHeaderCtrl::DeleteItem( int iItem )
    {
        if(iItem<0 || (UINT)iItem>=m_arrItems.GetCount()) return FALSE;

        int iOrder=m_arrItems[iItem].iOrder;
        if(m_arrItems[iItem].pszText) free(m_arrItems[iItem].pszText);
        m_arrItems.RemoveAt(iItem);
        //更新排序
        for(UINT i=0;i<m_arrItems.GetCount();i++)
        {
            if(m_arrItems[i].iOrder>iOrder)
                m_arrItems[i].iOrder--;
        }
        Invalidate();
        return TRUE;
    }

    void SHeaderCtrl::DeleteAllItems()
    {
        for(UINT i=0;i<m_arrItems.GetCount();i++)
        {
            if(m_arrItems[i].pszText) free(m_arrItems[i].pszText);
        }
        m_arrItems.RemoveAll();
        Invalidate();
    }

    void SHeaderCtrl::OnDestroy()
    {
        DeleteAllItems();
        __super::OnDestroy();
    }

    CRect SHeaderCtrl::GetItemRect( UINT iItem )
    {
        CRect    rcClient;
        GetClientRect(&rcClient);
        CRect rcItem(rcClient.left,rcClient.top,rcClient.left,rcClient.bottom);
        for(UINT i=0;i<=iItem && i<m_arrItems.GetCount();i++)
        {
            rcItem.left=rcItem.right;
            rcItem.right=rcItem.left+m_arrItems[i].cx;
        }
        return rcItem;
    }

    void SHeaderCtrl::RedrawItem( int iItem )
    {
        CRect rcItem=GetItemRect(iItem);
        IRenderTarget *pRT=GetRenderTarget(rcItem,OLEDC_PAINTBKGND);
        SPainter painter;
        BeforePaint(pRT,painter);
        DrawItem(pRT,rcItem,m_arrItems.GetData()+iItem);
        AfterPaint(pRT,painter);
        ReleaseRenderTarget(pRT);
    }

    void SHeaderCtrl::OnLButtonDown( UINT nFlags,CPoint pt )
    {
        SetCapture();
        m_ptClick=pt;
        m_dwHitTest=HitTest(pt);
        if(IsItemHover(m_dwHitTest))
        {
            if(m_bSortHeader)
            {
                m_arrItems[LOWORD(m_dwHitTest)].state=2;//pushdown
                RedrawItem(LOWORD(m_dwHitTest));
            }
        }else if(m_dwHitTest!=-1)
        {
            m_nAdjItemOldWidth=m_arrItems[LOWORD(m_dwHitTest)].cx;
        }
    }

    void SHeaderCtrl::OnLButtonUp( UINT nFlags,CPoint pt )
    {
        if(IsItemHover(m_dwHitTest))
        {
            if(m_bDragging)
            {//拖动表头项
                if(m_bItemSwapEnable)
                {
                    CDragWnd::EndDrag();
                    DeleteObject(m_hDragImg);
                    m_hDragImg=NULL;

                    m_arrItems[LOWORD(m_dwHitTest)].state=0;//normal

                    if(m_dwDragTo!=m_dwHitTest && IsItemHover(m_dwDragTo))
                    {
                        SHDITEM t=m_arrItems[LOWORD(m_dwHitTest)];
                        m_arrItems.RemoveAt(LOWORD(m_dwHitTest));
                        int nPos=LOWORD(m_dwDragTo);
                        if(nPos>LOWORD(m_dwHitTest)) nPos--;//要考虑将自己移除的影响
                        m_arrItems.InsertAt(LOWORD(m_dwDragTo),t);
                        //发消息通知宿主表项位置发生变化
                        EventHeaderItemSwap evt(this);
                        evt.iOldIndex=LOWORD(m_dwHitTest);
                        evt.iNewIndex=nPos;
                        FireEvent(evt);
                    }
                    m_dwHitTest=HitTest(pt);
                    m_dwDragTo=-1;
                    Invalidate();
                }
            }else
            {//点击表头项
                if(m_bSortHeader)
                {
                    m_arrItems[LOWORD(m_dwHitTest)].state=1;//hover
                    RedrawItem(LOWORD(m_dwHitTest));
                    EventHeaderClick evt(this);
                    evt.iItem=LOWORD(m_dwHitTest);
                    FireEvent(evt);
                }
            }
        }else if(m_dwHitTest!=-1)
        {//调整表头宽度，发送一个调整完成消息
            EventHeaderItemChanged evt(this);
            evt.iItem=LOWORD(m_dwHitTest);
            evt.nWidth=m_arrItems[evt.iItem].cx;
            FireEvent(evt);
        }
        m_bDragging=FALSE;
        ReleaseCapture();
    }

    void SHeaderCtrl::OnMouseMove( UINT nFlags,CPoint pt )
    {
        if(m_bDragging || nFlags&MK_LBUTTON)
        {
            if(!m_bDragging)
            {
                m_bDragging=TRUE;
                if(IsItemHover(m_dwHitTest) && m_bItemSwapEnable)
                {
                    m_dwDragTo=m_dwHitTest;
                    CRect rcItem=GetItemRect(LOWORD(m_dwHitTest));
                    DrawDraggingState(m_dwDragTo);
                    m_hDragImg=CreateDragImage(LOWORD(m_dwHitTest));
                    CPoint pt=m_ptClick-rcItem.TopLeft();
                    CDragWnd::BeginDrag(m_hDragImg,pt,0,128,LWA_ALPHA|LWA_COLORKEY);
                }
            }
            if(IsItemHover(m_dwHitTest))
            {
                if(m_bItemSwapEnable)
                {
                    DWORD dwDragTo=HitTest(pt);
                    CPoint pt2(pt.x,m_ptClick.y);
                    ClientToScreen(GetContainer()->GetHostHwnd(),&pt2);
                    if(IsItemHover(dwDragTo) && m_dwDragTo!=dwDragTo)
                    {
                        m_dwDragTo=dwDragTo;
                        STRACE(_T("\n!!! dragto %d"),LOWORD(dwDragTo));
                        DrawDraggingState(dwDragTo);
                    }
                    CDragWnd::DragMove(pt2);
                }
            }else if(m_dwHitTest!=-1)
            {//调节宽度
				if (!m_bFixWidth)
				{
					int cxNew = m_nAdjItemOldWidth + pt.x - m_ptClick.x;
					if (cxNew < 0) cxNew = 0;
					m_arrItems[LOWORD(m_dwHitTest)].cx = cxNew;
					Invalidate();
					GetContainer()->SwndUpdateWindow();//立即更新窗口
					//发出调节宽度消息
					EventHeaderItemChanging evt(this);
					evt.nWidth = cxNew;
					FireEvent(evt);
				}
            }
        }else
        {
            DWORD dwHitTest=HitTest(pt);
            if(dwHitTest!=m_dwHitTest)
            {
                if(m_bSortHeader)
                {
                    if(IsItemHover(m_dwHitTest))
                    {
                        WORD iHover=LOWORD(m_dwHitTest);
                        m_arrItems[iHover].state=0;
                        RedrawItem(iHover);
                    }
                    if(IsItemHover(dwHitTest))
                    {
                        WORD iHover=LOWORD(dwHitTest);
                        m_arrItems[iHover].state=1;//hover
                        RedrawItem(iHover);
                    }
                }
                m_dwHitTest=dwHitTest;
            }
        }
        
    }

    void SHeaderCtrl::OnMouseLeave()
    {
        if(!m_bDragging)
        {
            if(IsItemHover(m_dwHitTest) && m_bSortHeader)
            {
                m_arrItems[LOWORD(m_dwHitTest)].state=0;
                RedrawItem(LOWORD(m_dwHitTest));
            }
            m_dwHitTest=-1;
        }
    }

    BOOL SHeaderCtrl::CreateChildren( pugi::xml_node xmlNode )
    {
        pugi::xml_node xmlItems=xmlNode.child(L"items");
        if(!xmlItems) return FALSE;
        pugi::xml_node xmlItem=xmlItems.child(L"item");
        int iOrder=0;
        while(xmlItem)
        {
            SHDITEM item={0};
            item.mask=0xFFFFFFFF;
            item.iOrder=iOrder++;
            SStringT strTxt=S_CW2T(xmlItem.text().get());
            item.pszText=_tcsdup(strTxt);
            item.cchTextMax=strTxt.GetLength();
            item.cx=xmlItem.attribute(L"width").as_int(50);
            item.lParam=xmlItem.attribute(L"userData").as_uint(0);
            item.stFlag=(SHDSORTFLAG)xmlItem.attribute(L"sortFlag").as_uint(ST_NULL);
            m_arrItems.InsertAt(m_arrItems.GetCount(),item);
            xmlItem=xmlItem.next_sibling(L"item");
        }
        return TRUE;
    }

    BOOL SHeaderCtrl::OnSetCursor( const CPoint &pt )
    {
        if(m_bFixWidth) return FALSE;
        DWORD dwHit=HitTest(pt);
        if(HIWORD(dwHit)==LOWORD(dwHit)) return FALSE;
        HCURSOR hCursor=GETRESPROVIDER->LoadCursor(IDC_SIZEWE);
        SetCursor(hCursor);
        return TRUE;
    }

    DWORD SHeaderCtrl::HitTest( CPoint pt )
    {
        CRect    rcClient;
        GetClientRect(&rcClient);
        if(!rcClient.PtInRect(pt)) return -1;
        
        CRect rcItem(rcClient.left,rcClient.top,rcClient.left,rcClient.bottom);
        int nMargin=m_bSortHeader?CX_HDITEM_MARGIN:0;
        for(UINT i=0;i<m_arrItems.GetCount();i++)
        {
            if(m_arrItems[i].cx==0) continue;    //越过宽度为0的项

            rcItem.left=rcItem.right;
            rcItem.right=rcItem.left+m_arrItems[i].cx;
            if(pt.x<rcItem.left+nMargin)
            {
                int nLeft=i>0?i-1:0;
                return MAKELONG(nLeft,i);    
            }else if(pt.x<rcItem.right-nMargin)
            {
                return MAKELONG(i,i);
            }else if(pt.x<rcItem.right)
            {
                WORD nRight=(WORD)i+1;
                if(nRight>=m_arrItems.GetCount()) nRight=-1;//采用-1代表末尾
                return MAKELONG(i,nRight);
            }
        }
        return -1;
    }

    HBITMAP SHeaderCtrl::CreateDragImage( UINT iItem )
    {
        if(iItem>=m_arrItems.GetCount()) return NULL;
        CRect rcClient;
        GetClientRect(rcClient);
        CRect rcItem(0,0,m_arrItems[iItem].cx,rcClient.Height());
        
        CAutoRefPtr<IRenderTarget> pRT;
        GETRENDERFACTORY->CreateRenderTarget(&pRT,rcItem.Width(),rcItem.Height());
        BeforePaintEx(pRT);
        DrawItem(pRT,rcItem,m_arrItems.GetData()+iItem);
        
        HBITMAP hBmp=CreateBitmap(rcItem.Width(),rcItem.Height(),1,32,NULL);
        HDC hdc=GetDC(NULL);
        HDC hMemDC=CreateCompatibleDC(hdc);
        ::SelectObject(hMemDC,hBmp);
        HDC hdcSrc=pRT->GetDC(0);
        ::BitBlt(hMemDC,0,0,rcItem.Width(),rcItem.Height(),hdcSrc,0,0,SRCCOPY);
        pRT->ReleaseDC(hdcSrc);
        DeleteDC(hMemDC);
        ReleaseDC(NULL,hdc);
        return hBmp;
    }

    void SHeaderCtrl::DrawDraggingState(DWORD dwDragTo)
    {
        CRect rcClient;
        GetClientRect(&rcClient);
        IRenderTarget *pRT=GetRenderTarget(rcClient,OLEDC_PAINTBKGND);
        SPainter painter;
        BeforePaint(pRT,painter);
        CRect rcItem(rcClient.left,rcClient.top,rcClient.left,rcClient.bottom);
        int iDragTo=LOWORD(dwDragTo);
        int iDragFrom=LOWORD(m_dwHitTest);

        SArray<UINT> items;
        for(UINT i=0;i<m_arrItems.GetCount();i++)
        {
            if(i!=iDragFrom) items.Add(i);
        }
        items.InsertAt(iDragTo,iDragFrom);
        
        m_pSkinItem->Draw(pRT,rcClient,0);
        for(UINT i=0;i<items.GetCount();i++)
        {
            rcItem.left=rcItem.right;
            rcItem.right=rcItem.left+m_arrItems[items[i]].cx;
            if(items[i]!=iDragFrom)
                DrawItem(pRT,rcItem,m_arrItems.GetData()+items[i]);
        }
        AfterPaint(pRT,painter);
        ReleaseRenderTarget(pRT);
    }

    int SHeaderCtrl::GetTotalWidth()
    {
        int nRet=0;
        for(UINT i=0;i<m_arrItems.GetCount();i++)
            nRet+=m_arrItems[i].cx;
        return nRet;
    }

    int SHeaderCtrl::GetItemWidth( int iItem )
    {
        if(iItem<0 || (UINT)iItem>= m_arrItems.GetCount()) return -1;
        return m_arrItems[iItem].cx;
    }

    void SHeaderCtrl::OnActivateApp( BOOL bActive, DWORD dwThreadID )
    {
        if(m_bDragging)
        {
            if(m_bSortHeader && m_dwHitTest !=-1)
            {
                m_arrItems[LOWORD(m_dwHitTest)].state=0;//normal
            }
            m_dwHitTest = -1;
            
            CDragWnd::EndDrag();
            DeleteObject(m_hDragImg);
            m_hDragImg=NULL;
            m_bDragging=FALSE;
            ReleaseCapture();
            Invalidate();
        }
    }


}//end of namespace SOUI
